#include "../RasterizationRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    bool RasterizationRenderer::IsLensFlareEnabled() const
    {
        return renderFeatures.enableLensFlare;
    }

    RenderGraphTextureHandle RasterizationRenderer::DispatchLensFlarePass(
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
                lensFlareGhostTexture = builder.WriteTexture(lensFlareGhostTexture, RenderBackendResourceState::RenderTarget);

                builder.SetRenderTargetBinding(0, lensFlareGhostTexture, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Store);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(lensFlareTextureWidth), float(lensFlareTextureHeight));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, lensFlareTextureWidth, lensFlareTextureHeight);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(bloomTexture));
                    shaderConstants.BindScalar(2, lensFlareTextureSize.z);
                    shaderConstants.BindScalar(3, lensFlareTextureSize.w);

                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.colorBlendState.targetBlends[0] = RenderBackendColorBlendAttachmentState::Additive;

                    RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::DrawFullscreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::LensFlareGhost);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderConstants,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });

        //RenderGraphTextureDesc lensFlareTileCullingTextureDesc = RenderGraphTextureDesc::Create2D(
        //    (lensFlareTextureWidth + 1) / 2,
        //    (lensFlareTextureHeight + 1) / 2,
        //    RenderBackendTextureFormat::R11G11B10Float,
        //    RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        //RenderGraphTextureHandle tileCullingTexture = renderGraph.CreateTexture(lensFlareTileCullingTextureDesc, "LensFlareTileCullingTexture");

        //uint32 tileCount = lensFlareTileCullingTextureDesc.width * lensFlareTileCullingTextureDesc.height;

        //renderGraph.AddPass(
        //    std::format("LensFlareTileCulling (Compute, {}x{})", lensFlareTileCullingTextureDesc.width, lensFlareTileCullingTextureDesc.height),
        //    RenderGraphPassFlags::Compute,
        //    [&](RenderGraphBuilder& builder)
        //    {
        //        builder.ReadTexture(halfResolutionSceneColorTexture, RenderBackendResourceState::ShaderResource);

        //        tileCullingTexture = builder.WriteTexture(tileCullingTexture, RenderBackendResourceState::UnorderedAccess);

        //
        //        {
        //            uint32 threadGroupCountX = ComputeWorkGroupCount(lensFlareTileCullingTextureDesc.width, PostProcessingThreadGroupSizeX);
        //            uint32 threadGroupCountY = ComputeWorkGroupCount(lensFlareTileCullingTextureDesc.height, PostProcessingThreadGroupSizeY);

        //            RenderBackendPushConstantValues shaderConstants = {};
        //            shaderConstants.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(halfResolutionSceneColorTexture)));
        //            shaderConstants.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndextileCullingTexture), 0));

        //            RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LensFlareTileCulling);
        //            commandList.Dispatch2D(
        //                computeShader,
        //                shaderConstants,
        //                threadGroupCountX,
        //                groupCountY);
        //        };
        //    });

        //RenderGraphTextureDesc lensFlareGlareTextureDesc = RenderGraphTextureDesc::Create2D(
        //    lensFlareTextureWidth,
        //    lensFlareTextureHeight,
        //    RenderBackendTextureFormat::R11G11B10Float,
        //    RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        //RenderGraphTextureHandle lensFlareGlareTexture = renderGraph.CreateTexture(lensFlareGlareTextureDesc, "LensFlareGlareTexture");

        //renderGraph.AddPass(
        //    std::format("LensFlareGlare (Graphics, {}x{})", lensFlareGlareTextureDesc.width, lensFlareGlareTextureDesc.height),
        //    RenderGraphPassFlags::Graphics,
        //    [&](RenderGraphBuilder& builder)
        //    {
        //        builder.ReadTexture(tileCullingTexture, RenderBackendResourceState::ShaderResource);

        //        lensFlareGlareTexture = builder.WriteTexture(lensFlareGlareTexture, RenderBackendResourceState::RenderTarget);

        //        builder.BindColorTarget(0, lensFlareGlareTexture, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Load);

        //
        //        {
        //            RenderBackendViewport viewport(0.0f, 0.0f, (float)lensFlareTextureWidth, (float)lensFlareTextureHeight);
        //            commandList.SetViewports(&viewport, 1);

        //            RenderBackendScissor scissor(0, 0, lensFlareTextureWidth, lensFlareTextureHeight);
        //            commandList.SetScissors(&scissor, 1);

        //            RenderBackendPushConstantValues shaderConstants = {};
        //            shaderConstants.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(lensFlareGlareLUTTexture));
        //            shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(tileCullingTexture)));
        //            shaderConstants.PushConstants(0, (float)lensFlareTextureWidth);
        //            shaderConstants.PushConstants(1, (float)lensFlareTextureHeight);

        //            RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
        //            graphicsPipelineState.colorBlendState.targetBlends[0] = additiveColorBlendAttachmentStateRGB;

        //            RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::LensFlareGlare);

        //            commandList.Draw(
        //                graphicsShader,
        //                graphicsPipelineState,
        //                shaderConstants,
        //                4, tileCount * 3, 0, 0,
        //                RenderBackendPrimitiveTopology::TriangleStrip);
        //        };
        //    });

        RenderGraphTextureHandle lensFlareGradientTexture = defaultResources->ImportWhiteDummyTexture2D(renderGraph);
        RenderGraphTextureHandle lensFlareGlareTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);

        renderGraph.AddPass(
            std::format("LensFlareCombine (Graphics, {}x{})", outputTextureWidth, outputTextureHeight),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                lensFlareGhostTexture = builder.ReadTexture(lensFlareGhostTexture, RenderBackendResourceState::ShaderResource);
                lensFlareGlareTexture = builder.ReadTexture(lensFlareGlareTexture, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::RenderTarget);

                builder.SetRenderTargetBinding(0, outputTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(outputTextureWidth), float(outputTextureHeight));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, outputTextureWidth, outputTextureHeight);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(lensFlareGradientTexture));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(lensFlareGhostTexture));
                    shaderConstants.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(lensFlareGlareTexture));
                    shaderConstants.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(halfResolutionColorTexture));
                    shaderConstants.BindScalar(5, outputTextureSize.z);
                    shaderConstants.BindScalar(6, outputTextureSize.w);

                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.colorBlendState.targetBlends[0] = RenderBackendColorBlendAttachmentState::Additive;

                    RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::DrawFullscreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::LensFlareCombine);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderConstants,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });

        return outputTexture;
    }
}