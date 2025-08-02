#include "RasterizationRenderer.h"

namespace Horizon
{
    static constexpr uint32 ScreenSpaceLightShaftsDownsampleFactor = 4;
    static constexpr uint32 ScreenSpaceLightShaftsRadialBlurPassCount = 3;
    static constexpr uint32 ScreenSpaceLightShaftsRadialBlurSampleCount = 8;

    void RasterizationRenderer::RenderScreenSpaceLightShafts(
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
            RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

            Vector2f lightShaftsOrigin = Vector2f(clipSpaceLightPosition.x / clipSpaceLightPosition.w, clipSpaceLightPosition.y / clipSpaceLightPosition.w);
            lightShaftsOrigin = lightShaftsOrigin * Vector2f(0.5f, -0.5f) + 0.5f;

            Extent2D lightShaftsTextureSize = DownsampleExtent2D(renderResolution, ScreenSpaceLightShaftsDownsampleFactor);

            Vector2f aspectRatio = Vector2f(float(lightShaftsTextureSize.width) / float(lightShaftsTextureSize.height), float(lightShaftsTextureSize.height) / float(lightShaftsTextureSize.width));

            RenderGraphTextureDescription lightShaftsTextureDesc = RenderGraphTextureDescription::Create2D(
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
                    RenderGraphTextureHandle sceneColorTexture = builder.ReadTexture(intermediateResources.colorTexture, RenderBackendResourceState::ShaderResource);
                    RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::ShaderResource);
                    lightShaftsDownsampleOutputTexture = builder.WriteTexture(lightShaftsDownsampleOutputTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(lightShaftsTextureSize.width, 8);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(lightShaftsTextureSize.height, 8);
                        uint32 threadGroupCountZ = 1;

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneColorTexture));
                        pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                        pushConstantValues.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(lightShaftsDownsampleOutputTexture, 0));
                        pushConstantValues.OverrideShaderConstantValue(4, lightShaftsOrigin.x);
                        pushConstantValues.OverrideShaderConstantValue(5, lightShaftsOrigin.y);
                        pushConstantValues.OverrideShaderConstantValue(6, aspectRatio.x);
                        pushConstantValues.OverrideShaderConstantValue(7, aspectRatio.y);

                        RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::ScreenSpaceLightShaftsDownsample);

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
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
                        RenderGraphTextureHandle inputDepthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::ShaderResource);
                        RenderGraphTextureHandle historyColorTexture = builder.ReadTexture(lightShaftsTemporalFilteringOutputHistoryTexture, RenderBackendResourceState::ShaderResource);
                        RenderGraphTextureHandle outputColorTexture = lightShaftsTemporalFilteringOutputTexture = builder.WriteTexture(lightShaftsTemporalFilteringOutputTexture, RenderBackendResourceState::UnorderedAccess);

                        return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                        {
                            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(lightShaftsTextureSize.width, 8);
                            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(lightShaftsTextureSize.height, 8);
                            uint32 threadGroupCountZ = 1;

                            RenderBackendPushConstantValues pushConstantValues = {};
                            pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                            pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(inputColorTexture));
                            pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(inputDepthTexture));
                            pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(historyColorTexture));
                            pushConstantValues.BindTextureUAV(4, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputColorTexture, 0));

                            RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::ScreenSpaceLightShaftsTemporalFiltering);

                            commandList.Dispatch(
                                computeShader,
                                pushConstantValues,
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

                            RenderBackendPushConstantValues pushConstantValues = {};
                            pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                            pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(radialBlurInputTexture));
                            pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(radialBlurOutputTexture, 0));
                            pushConstantValues.OverrideShaderConstantValue(3, lightShaftsOrigin.x);
                            pushConstantValues.OverrideShaderConstantValue(4, lightShaftsOrigin.y);
                            pushConstantValues.OverrideShaderConstantValue(5, blurPassIndex);
                            pushConstantValues.OverrideShaderConstantValue(6, ScreenSpaceLightShaftsRadialBlurSampleCount);

                            RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::ScreenSpaceLightShaftsRadialBlur);

                            commandList.Dispatch(
                                computeShader,
                                pushConstantValues,
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
                    RenderGraphTextureHandle sceneColorTexture = intermediateResources.colorTexture;

                    builder.SetRenderTargetBinding(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                        graphicsPipelineState.depthStencilState.depthTestEnable = false;
                        graphicsPipelineState.colorBlendState.targetBlends[0] = RenderBackendColorBlendAttachmentState::AdditiveRGB;

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(lightShaftsTexture));
                        pushConstantValues.OverrideShaderConstantValue(2, lightShaftsIntensity);
                        pushConstantValues.OverrideShaderConstantValue(3, lightShaftsColor.x);
                        pushConstantValues.OverrideShaderConstantValue(4, lightShaftsColor.y);
                        pushConstantValues.OverrideShaderConstantValue(5, lightShaftsColor.z);

                        RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::DrawFullscreenQuadVS);
                        RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::ScreenSpaceLightShaftsComposition);

                        commandList.Draw(
                            vertexShader,
                            pixelShader,
                            graphicsPipelineState,
                            pushConstantValues,
                            3, 1, 0, 0,
                            RenderBackendPrimitiveTopology::TriangleList);
                    };
                });
        }
    }
}