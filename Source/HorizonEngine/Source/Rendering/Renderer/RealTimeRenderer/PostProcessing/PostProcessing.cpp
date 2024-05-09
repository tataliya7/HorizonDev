#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    void RealTimeRenderer::RenderPostProcessingEffects(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture;
        RenderGraphTextureHandle sceneDepthTexture = sceneTextures.sceneDepthTexture;
        RenderGraphTextureHandle motionVectorTexture = sceneTextures.motionVectorTexture;

        RenderGraphBufferHandle previousAutoExposureBuffer = renderGraph.ImportExternalBuffer(autoExposureBufferHistory.get());
        RenderGraphBufferHandle autoExposureBuffer = previousAutoExposureBuffer;

        const bool generateSceneColorMipChain = isBloomEnabled;

#if WITH_HORIZON_EDITOR
        const bool isEditorSelectionOutlineEnabled = true;
        const bool isEditorGizmosEnabled = true;
#endif

        const bool isVisualizePrimitiveIDEnabled = (view.debugViewMode == DebugViewMode::PrimitiveID);
        const bool isVisualizeMaterialIDEnabled = (view.debugViewMode == DebugViewMode::MaterialID);
        const bool isVisualizeWorldSpaceNormalEnabled = (view.debugViewMode == DebugViewMode::WorldSpaceNormal);
        const bool isVisualizeMotionVectorsEnabled = (view.debugViewMode == DebugViewMode::MotionVectors);
        const bool isVisualizeAmbientOcclusionEnabled = (view.debugViewMode == DebugViewMode::AmbientOcclusion);
        const bool isVisualizeShadowMaskEnabled = (view.debugViewMode == DebugViewMode::ShadowMask);

        bool depthOfFieldEnabled = settings.postProcessingSettings.dofScale > 0;
        if (depthOfFieldEnabled)
        {
            sceneColorTexture = AddDepthOfFieldPass(renderGraph, view, sceneColorTexture);
        }

        if (IsSuperResolutionEnabled())
        {
            if (IsFSR2Enabled())
            {

                sceneColorTexture = AddFSR2Pass(renderGraph, view, sceneColorTexture, sceneDepthTexture, motionVectorTexture);
            }
            else if (IsDLSSEnabled())
            {
                sceneColorTexture = AddDLSSPass(renderGraph, view, sceneColorTexture, sceneDepthTexture, motionVectorTexture);
            }
        }
        else
        {
            if (IsTemporalAAEnabled())
            {
                sceneColorTexture = AddTemporalSuperSamplingPass(renderGraph, view, sceneColorTexture, sceneDepthTexture, motionVectorTexture);
            }
            else if (IsDLAAEnabled())
            {
                sceneColorTexture = AddDLSSPass(renderGraph, view, sceneColorTexture, sceneDepthTexture, motionVectorTexture);
            }
        }

        if (false)
        {
            sceneColorTexture = AddMotionBlurPass(renderGraph, view);
        }

        //sceneColorTexture = renderGraph.ImportExternalTexture(localExposureTestTexture, localExposureTestTextureDesc, RenderBackendResourceState::ShaderResource, "Test");
        PostProcessingSceneColorMipChain sceneColorMipChain;
        if (generateSceneColorMipChain)
        {
            AddGenerateSceneColorMipChainPass(renderGraph, view, sceneColorTexture, &sceneColorMipChain);
        }

        if (isAutoExposureEnabled)
        {
            RenderGraphTextureHandle autoExposureHistogramTexture = AddAutoExposureBuildHistogramPass(renderGraph, view, sceneColorTexture);

            autoExposureBuffer = AddAutoExposureComputeExposurePass(renderGraph, view, autoExposureHistogramTexture, previousAutoExposureBuffer);
        }

        RenderGraphTextureHandle localExposureTexture = RenderGraphTextureHandle::Null;
        if (isLocalExposureEnabled)
        {
            localExposureTexture = AddLocalExposurePass(renderGraph, view, sceneColorTexture, RenderGraphTextureHandle::Null);
        }

        RenderGraphTextureHandle bloomTexture = RenderGraphTextureHandle::Null;
        if (isBloomEnabled)
        {
            if (isConvolutionBloomEnabled)
            {
                bloomTexture = AddConvolutionBloomPass(renderGraph, view, sceneColorMipChain);
            }
            else
            {
                bloomTexture = AddGaussianBloomPass(renderGraph, view, sceneColorMipChain);
            }

            if (isLensFlaresEnabled)
            {
                bloomTexture = AddLensFlaresPass(renderGraph, view, sceneColorMipChain.textures[1], bloomTexture);
            }
        }

        if (isToneMappingEnabled)
        {
            RenderGraphTextureHandle colorLUTTexture = AddColorLUTPass(renderGraph, view);

            bool outputInHDR = false;

            sceneColorTexture = AddToneMappingPass(renderGraph, view, sceneColorTexture, bloomTexture, autoExposureBuffer, colorLUTTexture, localExposureTexture, outputInHDR);
        }
        RenderGraphTextureHandle sceneColorTextureAfterToneMapping = sceneColorTexture;

#if WITH_HORIZON_EDITOR
        if (isEditorSelectionOutlineEnabled)
        {
            sceneColorTexture = AddEditorSelectionOutlinePass(renderGraph, view, sceneColorTexture);
        }

        /*if (isEditorGizmosEnabled)
        {
            sceneColorTexture = AddEditorGizmosPass(renderGraph, view, sceneColorTexture);
        }*/
#endif
        if (isVisualizePrimitiveIDEnabled)
        {
            sceneColorTexture = AddVisualizePrimitiveIDPass(renderGraph, view);
        }
        else if (isVisualizeMaterialIDEnabled)
        {
            sceneColorTexture = AddVisualizeMaterialIDPass(renderGraph, view);
        }
        else if (isVisualizeWorldSpaceNormalEnabled)
        {
            sceneColorTexture = AddVisualizeWorldSpaceNormalPass(renderGraph, view);
        }
        else if (isVisualizeMotionVectorsEnabled)
        {
            sceneColorTexture = AddVisualizeMotionVectorsPass(renderGraph, view);
        }
        else if (isVisualizeAmbientOcclusionEnabled)
        {
            sceneColorTexture = AddVisualizeAmbientOcclusionPass(renderGraph, view);
        }
        else if (isVisualizeShadowMaskEnabled)
        {
            sceneColorTexture = AddVisualizeShadowMaskPass(renderGraph, view, sceneColorTexture);
        }

        // TODO
        auto& finalTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();
        finalTextureData.finalTexture = sceneColorTexture;

        // TODO: distortion, screenshot

        if (false)
        {
            renderGraph.AddPass("DebugDrawLinesPass", RenderGraphPassFlags::Graphics,
                [&](RenderGraphBuilder& builder)
                {
                    auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                    auto& finalTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();

                    auto sceneDepthTexture = sceneTextures.sceneDepthTexture = builder.ReadWriteTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);
                    auto finalTexture = finalTextureData.finalTexture = builder.ReadWriteTexture(finalTextureData.finalTexture, RenderBackendResourceState::RenderTarget);

                    builder.BindColorTarget(0, finalTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);
                    builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                        graphicsPipelineState.rasterizationState.lineWidth = 2.0f;
                        graphicsPipelineState.depthStencilState.depthTestEnable = true;
                        graphicsPipelineState.depthStencilState.depthWriteEnable = true;
                        graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindBuffer(1, renderEngine->debugDrawLinesVertexBuffer, 0);

                        RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle(ShaderID::DebugDraw);
                        commandList.Draw(
                            graphicsShader,
                            graphicsPipelineState,
                            shaderArguments,
                            (uint32)renderEngine->debugDrawLinesVertices.size(),
                            1,
                            0,
                            0,
                            RenderBackendPrimitiveTopology::LineList);
                    };
                });
        }
    }
}