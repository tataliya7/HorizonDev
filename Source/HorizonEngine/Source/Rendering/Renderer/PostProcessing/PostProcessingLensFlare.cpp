#include "PostProcessingPipeline.h"

namespace Horizon
{
    RenderGraphTextureHandle PostProcessingPipeline::DispatchLensFlare(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle colorTexture,
        const PostProcessingColorPyramid& colorPyramid,
        RenderGraphTextureHandle bloomTexture)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "LensFlare");

        RenderGraphTextureHandle halfResolutionColorTexture = colorPyramid.textures[0];

        uint32 lensFlareTextureWidth = targetResolution.width / 2;
        uint32 lensFlareTextureHeight = targetResolution.height / 2;

        RenderGraphTextureHandle outputTexture = bloomTexture;
        uint32 outputTextureWidth = targetResolution.width / 2;
        uint32 outputTextureHeight = targetResolution.height / 2;
        const Vector4f outputTextureSize = Vector4f(outputTextureWidth, outputTextureHeight, 1.0f / outputTextureWidth, 1.0f / outputTextureHeight);

        RenderGraphTextureDescription lensFlareGhostTextureDesc = RenderGraphTextureDescription::Create2D(
            lensFlareTextureWidth,
            lensFlareTextureHeight,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle lensFlareGhostTexture = renderGraph.CreateTexture(lensFlareGhostTextureDesc, "LensFlareGhostTexture");

        const Vector4f lensFlareTextureSize = Vector4f(lensFlareTextureWidth, lensFlareTextureHeight, 1.0f / lensFlareTextureWidth, 1.0f / lensFlareTextureHeight);

        renderGraph.AddPass(
            std::format("LensFlareGhost (Graphics, {}x{})", lensFlareTextureWidth, lensFlareTextureHeight),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                bloomTexture = builder.ReadTexture(bloomTexture, RenderBackendResourceState::ShaderResource);

                builder.SetRenderTargetBinding(0, lensFlareGhostTexture, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Store);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(lensFlareTextureWidth), float(lensFlareTextureHeight));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, lensFlareTextureWidth, lensFlareTextureHeight);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(bloomTexture));
                    pushConstantValues.OverrideShaderConstantValue(2, lensFlareTextureSize.z);
                    pushConstantValues.OverrideShaderConstantValue(3, lensFlareTextureSize.w);

                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.colorBlendState.targetBlends[0] = RenderBackendColorBlendAttachmentState::Additive;

                    RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::DrawFullscreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::LensFlareGhost);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        pushConstantValues,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });

        RenderGraphTextureHandle lensFlareGradientTexture = defaultResources->ImportWhiteDummyTexture2D(renderGraph);
        RenderGraphTextureHandle lensFlareGlareTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);

        renderGraph.AddPass(
            std::format("LensFlareCombine (Graphics, {}x{})", outputTextureWidth, outputTextureHeight),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                lensFlareGhostTexture = builder.ReadTexture(lensFlareGhostTexture, RenderBackendResourceState::ShaderResource);
                lensFlareGlareTexture = builder.ReadTexture(lensFlareGlareTexture, RenderBackendResourceState::ShaderResource);

                builder.SetRenderTargetBinding(0, outputTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(outputTextureWidth), float(outputTextureHeight));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, outputTextureWidth, outputTextureHeight);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(lensFlareGradientTexture));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(lensFlareGhostTexture));
                    pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(lensFlareGlareTexture));
                    pushConstantValues.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(halfResolutionColorTexture));
                    pushConstantValues.OverrideShaderConstantValue(5, outputTextureSize.z);
                    pushConstantValues.OverrideShaderConstantValue(6, outputTextureSize.w);

                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.colorBlendState.targetBlends[0] = RenderBackendColorBlendAttachmentState::Additive;

                    RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::DrawFullscreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::LensFlareCombine);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        pushConstantValues,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });

        return outputTexture;
    }
}
