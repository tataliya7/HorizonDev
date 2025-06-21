#include "RealTimeRenderer.h"

#include "BendSSS/bend_sss_cpu.h"

namespace Horizon
{
    void DispatchScreenSpaceShadowsBend(
        RenderGraph& renderGraph,
        const ShaderCollection* shaderLibrary,
        const SceneView& view,
        const LightRenderObject& light,
        const Extent2D& renderResolution,
        RenderGraphTextureHandle& screenSpaceShadowMaskTexture)
    {
        const float surfaceThickness = light.screenSpaceShadowsSurfaceThickness;
        const float shadowContrast = light.screenSpaceShadowsShadowContrast;

        const Vector4f lightDirection = view.transformations.worldToClipMatrix * Vector4f(-light.GetDirection(), 0.0f);

        float lightProjection[4] = { lightDirection.x, lightDirection.y, lightDirection.z, lightDirection.w };
        int viewportSize[2] = { int(renderResolution.width), int(renderResolution.height) };
        int minRenderBounds[2] = { 0, 0 };
        int maxRenderBounds[2] = { viewportSize[0], viewportSize[1] };
        const Bend::DispatchList dispatchList = Bend::BuildDispatchList(lightProjection, viewportSize, minRenderBounds, maxRenderBounds);

        RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "BendSSSOutputTexture");

        renderGraph.AddPass(
            std::format("ClearBendSSSOutputTexture"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendTextureClearValue clearValue = RenderBackendTextureClearValue::Black;
                    RenderBackendTextureUAVDesc outputTextureUAVDesc = RenderBackendTextureUAVDesc::Create(resourceRegistry.GetRenderBackendTextureHandle(outputTexture), 0);
                    commandList.ClearTextureUAV(outputTextureUAVDesc, clearValue);
                };
            });

        for (int dispatchIndex = 0; dispatchIndex < dispatchList.DispatchCount; dispatchIndex++)
        {
            const Bend::DispatchData& dispatchData = dispatchList.Dispatch[dispatchIndex];

            renderGraph.AddPass(
                std::format("Bend SSS (Compute, {}x{})", outputTextureDesc.width, outputTextureDesc.height),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                    RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                    outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        uint32 threadGroupCountX = dispatchData.WaveCount[0];
                        uint32 threadGroupCountY = dispatchData.WaveCount[1];
                        uint32 threadGroupCountZ = dispatchData.WaveCount[2];

                        RenderBackendPushConstantValues shaderConstants = {};
                        shaderConstants.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                        shaderConstants.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));
                        shaderConstants.BindScalar(2, surfaceThickness);
                        shaderConstants.BindScalar(3, shadowContrast);
                        shaderConstants.BindScalar(4, dispatchList.LightCoordinate_Shader[0]);
                        shaderConstants.BindScalar(5, dispatchList.LightCoordinate_Shader[1]);
                        shaderConstants.BindScalar(6, dispatchList.LightCoordinate_Shader[2]);
                        shaderConstants.BindScalar(7, dispatchList.LightCoordinate_Shader[3]);
                        shaderConstants.BindScalar(8, dispatchData.WaveOffset_Shader[0]);
                        shaderConstants.BindScalar(9, dispatchData.WaveOffset_Shader[1]);
                        shaderConstants.BindScalar(10, 1.0f / float(renderResolution.width));
                        shaderConstants.BindScalar(11, 1.0f / float(renderResolution.height));

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::ScreenSpaceShadowsBend);

                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    };
                });
        }

        // @todo Spatial or temporal filter
        renderGraph.AddPass(
            std::format("ScreenSpaceShadowsComposition (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                outputTexture = builder.ReadTexture(outputTexture, RenderBackendResourceState::ShaderResource);
                screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::RenderTarget);

                builder.SetRenderTargetBinding(0, screenSpaceShadowMaskTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Min;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcAlphaBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstAlphaBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].alphaBlendOp = RenderBackendBlendOp::Min;
                    graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::R;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(outputTexture));
                    shaderConstants.BindScalar(1, 1.0f / float(renderResolution.width));
                    shaderConstants.BindScalar(2, 1.0f/ float(renderResolution.height));

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::DrawFullscreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::ScreenSpaceShadowsComposition);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderConstants,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }

    void RealTimeRenderer::DispatchScreenSpaceShadows(
        RenderGraph& renderGraph,
        const SceneView& view,
        const LightRenderObject& light)
    {
#if !HORIZON_CONFIGURATION_RELEASE
        if (!light.IsDistantLight())
        {
            LogWarning(GLogger, "Currently screen space shadows only supports distant light.");
            return;
        }
#endif

        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "ScreenSpaceShadows");

        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        RenderGraphTextureHandle& screenSpaceShadowMaskTexture = sceneTextures.shadowMaskTexture;

        //if ()
        //{
        //    DispatchScreenSpaceShadowsStochastic();
        //}
        //else
        {
            DispatchScreenSpaceShadowsBend(renderGraph, shaderCollection, view, light, renderResolution, screenSpaceShadowMaskTexture);

            sceneTextures.shadowMaskTexture = screenSpaceShadowMaskTexture;
        }
    }
}