#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::AddLensFlaresPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle halfResolutionSceneColorTexture,
        RenderGraphTextureHandle bloomTexture)
    {
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

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(halfResolutionSceneColorTexture)));
                    shaderArguments.PushConstants(0, (float)lensFlaresTextureWidth);
                    shaderArguments.PushConstants(1, (float)lensFlaresTextureHeight);

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.colorBlendState.targetBlends[0] = additiveColorBlendAttachmentStateRGB;

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle(ShaderID::LensFlaresGhost);
                    commandList.Draw(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderArguments,
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
                    uint32 groupCountX = ComputeWorkGroupCount(lensFlaresTileCullingTextureDesc.width, PostProcessingThreadGroupCountX);
                    uint32 groupCountY = ComputeWorkGroupCount(lensFlaresTileCullingTextureDesc.height, PostProcessingThreadGroupCountY);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(halfResolutionSceneColorTexture)));
                    shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(tileCullingTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::LensFlaresTileCulling);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
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

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(lensFlaresGlareLUTTexture));
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(tileCullingTexture)));
                    shaderArguments.PushConstants(0, (float)lensFlaresTextureWidth);
                    shaderArguments.PushConstants(1, (float)lensFlaresTextureHeight);

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.colorBlendState.targetBlends[0] = additiveColorBlendAttachmentStateRGB;

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle(ShaderID::LensFlaresGlare);
                    commandList.Draw(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderArguments,
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

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(lensFlaresGradiantLUTTexture));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(lensFlaresGhostTexture)));
                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(lensFlaresGlareTexture)));
                    shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(halfResolutionSceneColorTexture)));
                    shaderArguments.PushConstants(0, (float)bloomTextureWidth);
                    shaderArguments.PushConstants(1, (float)bloomTextureHeight);

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.colorBlendState.targetBlends[0] = additiveColorBlendAttachmentStateRGB;

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle(ShaderID::LensFlaresCombine);
                    commandList.Draw(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderArguments,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });

        return lensFlaresTexture;
    }
}