#include "RealTimeRenderer.h"

namespace Horizon
{
    static constexpr uint32 ScreenSpaceLightShaftsDownsampleFactor = 4;
    static constexpr uint32 ScreenSpaceLightShaftsRadialBlurPassCount = 3;
    static constexpr uint32 ScreenSpaceLightShaftsRadialBlurSampleCount = 8;

    bool RealTimeRenderer::IsScreenSpaceLightShaftsEnabled() const
    {
        return features.enableScreenSpaceLightShafts;
    }

    void RealTimeRenderer::RenderScreenSpaceLightShafts(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "ScreenSpaceLightShafts");

        RenderScene* scene = view.scene;

        // Currently, only atmospheric light is supported.
        LightRenderObject* light = scene->GetAtmosphericLight();

        const float lightShaftsIntensity = light->GetLightShaftsIntensity();
        const Vector3f lightShaftsColor = light->GetLightShaftsColor();

        const Vector3f lightDirection = light->GetDirection();
        Vector3f lightPosition = view.cameraPosition - lightDirection;
        Vector4f clipSpaceLightPosition = view.transformations.worldToClipMatrix * Vector4f(lightPosition.x, lightPosition.y, lightPosition.z, 1.0f);

        const bool shouldRenderLightShafts =
            light->IsLightShaftsEnabled() &&
            lightShaftsIntensity > 0.0f &&
            lightShaftsColor != ZeroVector3f &&
            clipSpaceLightPosition.w > 0;

        if (shouldRenderLightShafts)
        {
            RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

            Vector2f lightShaftsOrigin = Vector2f(clipSpaceLightPosition.x / clipSpaceLightPosition.w, clipSpaceLightPosition.y / clipSpaceLightPosition.w);
            lightShaftsOrigin = lightShaftsOrigin * Vector2f(0.5f, -0.5f) + 0.5f;

            Extent2D lightShaftsTextureSize = DownsampleExtent2D(renderResolution, ScreenSpaceLightShaftsDownsampleFactor);

            Vector2f aspectRatio = Vector2f(float(lightShaftsTextureSize.width) / float(lightShaftsTextureSize.height), float(lightShaftsTextureSize.height) / float(lightShaftsTextureSize.width));

            RenderGraphTextureDesc lightShaftsTextureDesc = RenderGraphTextureDesc::Create2D(
                lightShaftsTextureSize.width,
                lightShaftsTextureSize.height,
                RenderBackendTextureFormat::R11G11B10Float,
                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
            RenderGraphTextureHandle lightShaftsDownsampleOutputTexture = renderGraph.CreateTexture(lightShaftsTextureDesc, "LightShaftsDownsampleOutputTexture");

            renderGraph.AddPass(
                std::format("ScreenSpaceLightShaftsDownsample (Compute, {}x{})", lightShaftsTextureSize.width, lightShaftsTextureSize.height),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    RenderGraphTextureHandle sceneColorTexture = builder.ReadTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::ShaderResource);
                    RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                    lightShaftsDownsampleOutputTexture = builder.WriteTexture(lightShaftsDownsampleOutputTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(lightShaftsTextureSize.width, 8);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(lightShaftsTextureSize.height, 8);
                        uint32 threadGroupCountZ = 1;

                        RenderBackendPushConstantValues shaderConstants = {};
                        shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneColorTexture));
                        shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                        shaderConstants.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(lightShaftsDownsampleOutputTexture, 0));
                        shaderConstants.BindScalar(4, lightShaftsOrigin.x);
                        shaderConstants.BindScalar(5, lightShaftsOrigin.y);
                        shaderConstants.BindScalar(6, aspectRatio.x);
                        shaderConstants.BindScalar(7, aspectRatio.y);

                        RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::ScreenSpaceLightShaftsDownsample);

                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    };
                });

            bool useTemporalFiltering = true;

            // Temporal filtering
            if (useTemporalFiltering)
            {
                RenderGraphTextureHandle lightShaftsTemporalFilteringOutputTexture = renderGraph.CreateTexture(lightShaftsTextureDesc, "LightShaftsTemporalFilteringOutputTexture");

                RenderGraphTextureHandle lightShaftsTemporalFilteringOutputHistoryTexture = renderGraph.ImportExternalTexture(historyFrame.screenSpaceLightShaftsTemporalFilteringTexture, "LightShaftsTemporalFilteringOutputHistoryTexture");
                if (!lightShaftsTemporalFilteringOutputHistoryTexture) // @todo Size changed
                {
                    lightShaftsTemporalFilteringOutputHistoryTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
                }

                renderGraph.AddPass(
                    std::format("ScreenSpaceLightShaftsTemporalFiltering (Compute, {}x{})", lightShaftsTextureSize.width, lightShaftsTextureSize.height),
                    RenderGraphPassFlags::Compute,
                    [&](RenderGraphBuilder& builder)
                    {
                        RenderGraphTextureHandle inputColorTexture = builder.ReadTexture(lightShaftsDownsampleOutputTexture, RenderBackendResourceState::ShaderResource);
                        RenderGraphTextureHandle inputDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                        RenderGraphTextureHandle historyColorTexture = builder.ReadTexture(lightShaftsTemporalFilteringOutputHistoryTexture, RenderBackendResourceState::ShaderResource);
                        RenderGraphTextureHandle outputColorTexture = lightShaftsTemporalFilteringOutputTexture = builder.WriteTexture(lightShaftsTemporalFilteringOutputTexture, RenderBackendResourceState::UnorderedAccess);

                        return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                        {
                            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(lightShaftsTextureSize.width, 8);
                            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(lightShaftsTextureSize.height, 8);
                            uint32 threadGroupCountZ = 1;

                            RenderBackendPushConstantValues shaderConstants = {};
                            shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                            shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(inputColorTexture));
                            shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(inputDepthTexture));
                            shaderConstants.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(historyColorTexture));
                            shaderConstants.BindTextureUAV(4, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputColorTexture, 0));

                            RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::ScreenSpaceLightShaftsTemporalFiltering);

                            commandList.Dispatch(
                                computeShader,
                                shaderConstants,
                                threadGroupCountX,
                                threadGroupCountY,
                                threadGroupCountZ);
                        };
                    });

                renderGraph.ExportTextureDeferred(lightShaftsTemporalFilteringOutputTexture, &historyFrame.screenSpaceLightShaftsTemporalFilteringTexture);

                lightShaftsDownsampleOutputTexture = lightShaftsTemporalFilteringOutputTexture;
            }

            RenderGraphTextureHandle radialBlurInputTexture = lightShaftsDownsampleOutputTexture;
            RenderGraphTextureHandle radialBlurOutputTexture = radialBlurInputTexture;

            for (uint32 blurPassIndex = 0; blurPassIndex < ScreenSpaceLightShaftsRadialBlurPassCount; blurPassIndex++)
            {
                radialBlurOutputTexture = renderGraph.CreateTexture(lightShaftsTextureDesc, "LightShaftsRadialBlurOutputTexture");

                renderGraph.AddPass(
                    std::format("ScreenSpaceLightShaftsRadialBlur (Compute, {}x{}, PassIndex={})", lightShaftsTextureSize.width, lightShaftsTextureSize.height, blurPassIndex),
                    RenderGraphPassFlags::Compute,
                    [&](RenderGraphBuilder& builder)
                    {
                        radialBlurInputTexture = builder.ReadTexture(radialBlurInputTexture, RenderBackendResourceState::ShaderResource);
                        radialBlurOutputTexture = builder.WriteTexture(radialBlurOutputTexture, RenderBackendResourceState::UnorderedAccess);

                        return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                        {
                            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(lightShaftsTextureSize.width, 8);
                            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(lightShaftsTextureSize.height, 8);
                            uint32 threadGroupCountZ = 1;

                            RenderBackendPushConstantValues shaderConstants = {};
                            shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                            shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(radialBlurInputTexture));
                            shaderConstants.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(radialBlurOutputTexture, 0));
                            shaderConstants.BindScalar(3, lightShaftsOrigin.x);
                            shaderConstants.BindScalar(4, lightShaftsOrigin.y);
                            shaderConstants.BindScalar(5, blurPassIndex);
                            shaderConstants.BindScalar(6, ScreenSpaceLightShaftsRadialBlurSampleCount);

                            RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::ScreenSpaceLightShaftsRadialBlur);

                            commandList.Dispatch(
                                computeShader,
                                shaderConstants,
                                threadGroupCountX,
                                threadGroupCountY,
                                threadGroupCountZ);
                        };
                    });

                radialBlurInputTexture = radialBlurOutputTexture;
            }

            renderGraph.AddPass(
                std::format("ScreenSpaceLightShaftsComposition (Compute, {}x{})", renderResolution.width, renderResolution.height),
                RenderGraphPassFlags::Graphics,
                [&](RenderGraphBuilder& builder)
                {
                    RenderGraphTextureHandle lightShaftsTexture = builder.ReadTexture(radialBlurOutputTexture, RenderBackendResourceState::ShaderResource);
                    RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                    builder.SetRenderTargetBinding(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                        graphicsPipelineState.depthStencilState.depthTestEnable = false;
                        graphicsPipelineState.colorBlendState.targetBlends[0] = RenderBackendColorBlendAttachmentState::AdditiveRGB;

                        RenderBackendPushConstantValues shaderConstants = {};
                        shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(lightShaftsTexture));
                        shaderConstants.BindScalar(2, lightShaftsIntensity);
                        shaderConstants.BindScalar(3, lightShaftsColor.x);
                        shaderConstants.BindScalar(4, lightShaftsColor.y);
                        shaderConstants.BindScalar(5, lightShaftsColor.z);

                        RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::DrawFullscreenQuadVS);
                        RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::ScreenSpaceLightShaftsComposition);

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
    }
}