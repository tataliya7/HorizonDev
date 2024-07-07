#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    bool RealTimeRenderer::IsLensFlaresEnabled() const
    {
        return features.enableLensFlares;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddLensFlaresPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle halfResolutionSceneColorTexture,
        RenderGraphTextureHandle bloomTexture)
    {
        return RenderGraphTextureHandle::Null;
    }
#if 0
        uint32 lensFlaresTextureWidth = targetResolution.width / 4;
        uint32 lensFlaresTextureHeight = targetResolution.height / 4;

        RenderGraphTextureHandle lensFlaresTexture = bloomTexture;
        uint32 bloomTextureWidth = targetResolution.width / 2;
        uint32 bloomTextureHeight = targetResolution.height / 2;

        RenderGraphTextureDesc lensFlaresGhostTextureDesc = RenderGraphTextureDesc::Create2D(
            lensFlaresTextureWidth,
            lensFlaresTextureHeight,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle lensFlaresGhostTexture = renderGraph.CreateTexture(lensFlaresGhostTextureDesc, "LensFlaresGhostTexture");

        renderGraph.AddPass(std::format("LensFlaresGhost (Graphics, {}x{})", lensFlaresTextureWidth, lensFlaresTextureHeight), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(halfResolutionSceneColorTexture, RenderBackendResourceState::ShaderResource);

                lensFlaresGhostTexture = builder.WriteTexture(lensFlaresGhostTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, lensFlaresGhostTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, (float)lensFlaresTextureWidth, (float)lensFlaresTextureHeight);
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, lensFlaresTextureWidth, lensFlaresTextureHeight);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(halfResolutionSceneColorTexture)));
                    shaderConstants.PushConstants(0, (float)lensFlaresTextureWidth);
                    shaderConstants.PushConstants(1, (float)lensFlaresTextureHeight);

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.colorBlendState.targetBlends[0] = additiveColorBlendAttachmentStateRGB;

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::LensFlaresGhost);
                    commandList.Draw(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderConstants,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });

        RenderGraphTextureDesc lensFlaresTileCullingTextureDesc = RenderGraphTextureDesc::Create2D(
            (lensFlaresTextureWidth + 1) / 2,
            (lensFlaresTextureHeight + 1) / 2,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle tileCullingTexture = renderGraph.CreateTexture(lensFlaresTileCullingTextureDesc, "LensFlaresTileCullingTexture");

        uint32 tileCount = lensFlaresTileCullingTextureDesc.width * lensFlaresTileCullingTextureDesc.height;

        // TODO: Use mesh shader instead
        renderGraph.AddPass(std::format("LensFlaresTileCulling (Compute, {}x{})", lensFlaresTileCullingTextureDesc.width, lensFlaresTileCullingTextureDesc.height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(halfResolutionSceneColorTexture, RenderBackendResourceState::ShaderResource);

                tileCullingTexture = builder.WriteTexture(tileCullingTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeWorkGroupCount(lensFlaresTileCullingTextureDesc.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeWorkGroupCount(lensFlaresTileCullingTextureDesc.height, PostProcessingThreadGroupSizeY);

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindTextureSRV(0, registry.GetTextureSRVBindlessResourceDescriptorIndex(halfResolutionSceneColorTexture)));
                    shaderConstants.BindTextureUAV(1, registry.GetTextureUAVBindlessResourceDescriptorIndextileCullingTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LensFlaresTileCulling);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        groupCountY);
                };
            });

        RenderGraphTextureDesc lensFlaresGlareTextureDesc = RenderGraphTextureDesc::Create2D(
            lensFlaresTextureWidth,
            lensFlaresTextureHeight,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle lensFlaresGlareTexture = renderGraph.CreateTexture(lensFlaresGlareTextureDesc, "LensFlaresGlareTexture");

        renderGraph.AddPass(std::format("LensFlaresGlare (Graphics, {}x{})", lensFlaresGlareTextureDesc.width, lensFlaresGlareTextureDesc.height), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(tileCullingTexture, RenderBackendResourceState::ShaderResource);

                lensFlaresGlareTexture = builder.WriteTexture(lensFlaresGlareTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, lensFlaresGlareTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, (float)lensFlaresTextureWidth, (float)lensFlaresTextureHeight);
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, lensFlaresTextureWidth, lensFlaresTextureHeight);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(lensFlaresGlareLUTTexture));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(tileCullingTexture)));
                    shaderConstants.PushConstants(0, (float)lensFlaresTextureWidth);
                    shaderConstants.PushConstants(1, (float)lensFlaresTextureHeight);

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.colorBlendState.targetBlends[0] = additiveColorBlendAttachmentStateRGB;

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::LensFlaresGlare);
                    commandList.Draw(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderConstants,
                        4, tileCount * 3, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleStrip);
                };
            });

        renderGraph.AddPass(std::format("LensFlaresCombine (Graphics, {}x{})", bloomTextureWidth, bloomTextureHeight), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(lensFlaresGhostTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(lensFlaresGlareTexture, RenderBackendResourceState::ShaderResource);

                lensFlaresTexture = builder.WriteTexture(lensFlaresTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, lensFlaresTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, (float)bloomTextureWidth, (float)bloomTextureHeight);
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, bloomTextureWidth, bloomTextureHeight);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(lensFlaresGradiantLUTTexture));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(lensFlaresGhostTexture)));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(lensFlaresGlareTexture)));
                    shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(halfResolutionSceneColorTexture)));
                    shaderConstants.PushConstants(0, (float)bloomTextureWidth);
                    shaderConstants.PushConstants(1, (float)bloomTextureHeight);

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.colorBlendState.targetBlends[0] = additiveColorBlendAttachmentStateRGB;

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::LensFlaresCombine);
                    commandList.Draw(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderConstants,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });

        return lensFlaresTexture;
    }
#endif
}