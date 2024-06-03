#include "PostProcessing.h"
#include "../RealTimeRenderer.h"
#include "../TemporalSuperSampling.h"

namespace Horizon
{
    void RealTimeRenderer::RenderPostProcessingEffects(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture;
        RenderGraphTextureHandle sceneDepthTexture = sceneTextures.sceneDepthTexture;
        RenderGraphTextureHandle motionVectorTexture = sceneTextures.motionVectorTexture;

        RenderGraphBufferHandle previousAutoExposureBuffer = renderGraph.ImportExternalBuffer(historyFrame.autoExposureBuffer, "PreviousAutoExposureBuffer");
        RenderGraphBufferHandle autoExposureBuffer = previousAutoExposureBuffer;

        const bool shouldRenderSceneColorPyramid = IsBloomEnabled();

#if HORIZON_EDITOR
        const bool isEditorSelectionOutlineEnabled = true;
        const bool isEditorGizmosEnabled = true;
#endif

        const bool isVisualizePrimitiveIDEnabled         = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::PrimitiveID);
        const bool isVisualizeMaterialIDEnabled          = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::MaterialID);
        const bool isVisualizeWorldSpaceNormalEnabled    = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::WorldSpaceNormal);
        const bool isVisualizeMotionVectorsEnabled       = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::MotionVectors);
        const bool isVisualizeAmbientOcclusionEnabled    = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::AmbientOcclusion);
        const bool isVisualizeShadowMaskEnabled          = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::ShadowMask);

        if (IsDepthOfFieldEnabled())
        {
            sceneColorTexture = AddDepthOfFieldPass(renderGraph, view, sceneColorTexture);
        }

        if (IsSuperResolutionEnabled())
        {
            if (true) // Currently, only third party temporal super sampling methods are supported.
            {
                TemporalSuperSamplingDispatchParameters tssDispatchParameters;
                tssDispatchParameters.colorTexture = sceneColorTexture;
                tssDispatchParameters.depthTexture = sceneDepthTexture;
                tssDispatchParameters.motionVectorTexture = motionVectorTexture;
                sceneColorTexture = DispatchCustomTemporalSuperSampling(renderGraph, view, tssDispatchParameters);
            }
        }

        if (IsMotionBlurEnabled())
        {
            sceneColorTexture = AddMotionBlurPass(renderGraph, view);
        }

        //sceneColorTexture = renderGraph.ImportExternalTexture(localExposureTestTexture, localExposureTestTextureDesc, RenderBackendResourceState::ShaderResource, "Test");
        PostProcessingSceneColorMipChain sceneColorMipChain;
        if (shouldRenderSceneColorPyramid)
        {
            RenderSceneColorPyramid(renderGraph, view, sceneColorTexture, &sceneColorMipChain);
        }

        // The auto exposure pass is always executed.
        // When fixed exposure is enabled, the auto exposure pass will force output the specified exposure.
        {
            RenderGraphTextureHandle autoExposureHistogramTexture = AddAutoExposureBuildHistogramPass(renderGraph, view, sceneColorTexture);

            autoExposureBuffer = AddAutoExposureComputeExposurePass(renderGraph, view, autoExposureHistogramTexture, previousAutoExposureBuffer);
        }

        RenderGraphTextureHandle localExposureTexture = RenderGraphTextureHandle::Null;
        if (IsLocalExposureEnabled())
        {
            localExposureTexture = AddLocalExposurePass(renderGraph, view, sceneColorTexture, RenderGraphTextureHandle::Null);
        }

        RenderGraphTextureHandle bloomTexture = RenderGraphTextureHandle::Null;
        if (IsBloomEnabled())
        {
            if (IsGaussianBloomEnabled())
            {
                bloomTexture = DispatchGaussianBloom(renderGraph, view, sceneColorMipChain.textures[0]);
            }
            else
            {
                assert(IsConvolutionBloomEnabled());
                bloomTexture = DispatchConvolutionBloom(renderGraph, view, sceneColorMipChain.textures[0]);
            }

            if (IsLensFlaresEnabled())
            {
                bloomTexture = AddLensFlaresPass(renderGraph, view, sceneColorMipChain.textures[1], bloomTexture);
            }
        }

        if (true) // Tone mapping is always enabled.
        {
            RenderGraphTextureHandle colorLUTTexture = AddColorLUTPass(renderGraph, view);

            bool outputInHDR = false;

            sceneColorTexture = AddToneMappingPass(renderGraph, view, sceneColorTexture, bloomTexture, colorLUTTexture, localExposureTexture, autoExposureBuffer, outputInHDR);
        }

        RenderGraphTextureHandle sceneColorTextureAfterToneMapping = sceneColorTexture;

#if WITH_HORIZON_EDITOR
        // if (isEditorSelectionOutlineEnabled)
        // {
        //     sceneColorTexture = AddEditorSelectionOutlinePass(renderGraph, view, sceneColorTexture);
        // }
        //
        // if (isEditorGizmosEnabled)
        // {
        //     sceneColorTexture = AddEditorGizmosPass(renderGraph, view, sceneColorTexture);
        // }
#endif
        //
        // if (isVisualizePrimitiveIDEnabled)
        // {
        //     sceneColorTexture = AddVisualizePrimitiveIDPass(renderGraph, view);
        // }
        // if (isVisualizeMaterialIDEnabled)
        // {
        //     sceneColorTexture = AddVisualizeMaterialIDPass(renderGraph, view);
        // }
        // if (isVisualizeWorldSpaceNormalEnabled)
        // {
        //     sceneColorTexture = AddVisualizeWorldSpaceNormalPass(renderGraph, view);
        // }
        // if (isVisualizeMotionVectorsEnabled)
        // {
        //     sceneColorTexture = AddVisualizeMotionVectorsPass(renderGraph, view);
        // }
        // if (isVisualizeAmbientOcclusionEnabled)
        // {
        //     sceneColorTexture = AddVisualizeAmbientOcclusionPass(renderGraph, view);
        // }
        // if (isVisualizeShadowMaskEnabled)
        // {
        //     sceneColorTexture = AddVisualizeShadowMaskPass(renderGraph, view, sceneColorTexture);
        // }

        sceneTextures.hudLessColorTexture = sceneColorTexture;

        // TODO: distortion, screenshot

#if 0
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
                        shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindBuffer(1, renderEngine->debugDrawLinesVertexBuffer, 0);

                        RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::DebugDraw);
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
#endif
    }
}