#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::RenderSubsurfaceScattering(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        const auto& sceneColorTextureDesc = sceneTextures.sceneColorTextureDesc;

        const uint32 width = sceneColorTextureDesc.width;
        const uint32 height = sceneColorTextureDesc.height;

        const uint32 SubsurfaceScatteringTileSize = 16;

        const uint32 tileCountX = ComputeWorkGroupCount(width, SubsurfaceScatteringTileSize);
        const uint32 tileCountY = ComputeWorkGroupCount(height, SubsurfaceScatteringTileSize);
        const uint32 tileCount = tileCountX * tileCountY;

        RenderGraphTextureHandle subsurfaceScatteringTexture = renderGraph.CreateTexture(sceneColorTextureDesc, "SubsurfaceScatteringTexture");
        RenderGraphTextureHandle subsurfaceScatteringColorTexture = renderGraph.CreateTexture(sceneColorTextureDesc, "SubsurfaceScatteringColorTexture");

        RenderGraphBufferHandle tileCountBuffer = renderGraph.CreateBuffer(RenderGraphBufferDesc::CreateByteAddress(sizeof(uint32)), "SubsurfaceScatteringTileCountBuffer");
        RenderGraphBufferHandle tileDataBuffer = renderGraph.CreateBuffer(RenderGraphBufferDesc::CreateByteAddress(sizeof(uint32) * 2 * tileCount), "SubsurfaceScatteringTileDataBuffer");

        RenderGraphBufferHandle dispatchIndirectArgumentBuffer = renderGraph.CreateBuffer(RenderGraphBufferDesc::CreateIndirectArguments(sizeof(RenderBackendDispatchIndirectArguments), 1), "SubsurfaceScatteringDispatchIndirectArgumentBuffer");
        uint64 dispatchIndirectArgumentBufferOffset = 0;

        RenderGraphBufferHandle drawIndirectArgumentBuffer = renderGraph.CreateBuffer(RenderGraphBufferDesc::CreateIndirectArguments(sizeof(RenderBackendDrawIndirectArguments), 1), "SubsurfaceScatteringDrawIndirectArgumentBuffer");
        uint64 drawIndirectArgumentBufferOffset = 0;

        renderGraph.AddPass(std::format("SubsurfaceScatteringSetup (Compute)"), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                tileCountBuffer = builder.WriteBuffer(tileCountBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, registry.GetRenderBackendBufferHandle(tileCountBuffer), 0);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::SubsurfaceScatteringSetup);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        1,
                        1,
                        1);
                };
            });

        renderGraph.AddPass(std::format("SubsurfaceScatteringClassifyTiles (Compute, {}x{})", renderResolution.width, renderResolution.height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto sceneColorTexture = builder.ReadTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::ShaderResource);
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);

                tileCountBuffer = builder.WriteBuffer(tileCountBuffer, RenderBackendResourceState::UnorderedAccess);
                tileDataBuffer = builder.WriteBuffer(tileDataBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(renderResolution.width, 16);
                    uint32 groupCountY = ComputeWorkGroupCount(renderResolution.height, 16);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                    shaderArguments.BindBuffer(3, registry.GetRenderBackendBufferHandle(tileCountBuffer), 0);
                    shaderArguments.BindBuffer(4, registry.GetRenderBackendBufferHandle(tileDataBuffer), 0);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::SubsurfaceScatteringClassifyTiles);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
                };
            });

        renderGraph.AddPass(std::format("SubsurfaceScatteringBuildIndirectArguments (Compute)"), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadBuffer(tileCountBuffer, RenderBackendResourceState::ShaderResource);

                dispatchIndirectArgumentBuffer = builder.WriteBuffer(dispatchIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);
                drawIndirectArgumentBuffer = builder.WriteBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, registry.GetRenderBackendBufferHandle(tileCountBuffer), 0);
                    shaderArguments.BindBuffer(1, registry.GetRenderBackendBufferHandle(dispatchIndirectArgumentBuffer), 0);
                    shaderArguments.BindBuffer(2, registry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer), 0);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::SubsurfaceScatteringBuildIndirectArguments);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        1,
                        1,
                        1);
                };
            });

        renderGraph.AddPass(std::format("SubsurfaceScatteringClearTextureUAV (Compute)"), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                subsurfaceScatteringColorTexture = builder.WriteTexture(subsurfaceScatteringColorTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.ClearTextureUAV(RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(subsurfaceScatteringColorTexture)), RenderBackendTextureClearValue::Black);
                };
            });

#if 0
        renderGraph.AddPass(std::format("SubsurfaceScatteringSampleDiffusionProfile (Compute, Tiled)"), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadBuffer(dispatchIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);
                builder.ReadBuffer(tileCountBuffer, RenderBackendResourceState::ShaderResource);

                argumentBuffer = builder.WriteBuffer(argumentBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, registry.GetRenderBackendBufferHandle(tileCountBuffer), 0);
                    shaderArguments.BindBuffer(1, registry.GetRenderBackendBufferHandle(argumentBuffer), 0);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::SubsurfaceScatteringSampleDiffusionProfile);
                    commandList.DispatchIndirect(
                        computeShader,
                        shaderArguments,
                        registry.GetRenderBackendBufferHandle(dispatchIndirectArgumentBuffer),
                        0);
                };
            });

        renderGraph.AddPass(std::format("SubsurfaceScatteringComputeVariance (Compute, Tiled)"), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadBuffer(dispatchIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);
                builder.ReadBuffer(tileCountBuffer, RenderBackendResourceState::ShaderResource);

                argumentBuffer = builder.WriteBuffer(argumentBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, registry.GetRenderBackendBufferHandle(tileCountBuffer), 0);
                    shaderArguments.BindBuffer(1, registry.GetRenderBackendBufferHandle(argumentBuffer), 0);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::SubsurfaceScatteringComputeVariance);
                    commandList.DispatchIndirect(
                        computeShader,
                        shaderArguments,
                        registry.GetRenderBackendBufferHandle(dispatchIndirectArgumentBuffer),
                        0);
                };
            });
#endif

        renderGraph.AddPass(std::format("SubsurfaceScatteringRecombine (Graphics, Tiled)"), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);
                builder.ReadBuffer(tileDataBuffer, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(subsurfaceScatteringTexture, RenderBackendResourceState::ShaderResource);

                auto sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindBuffer(1, registry.GetRenderBackendBufferHandle(tileDataBuffer), 0);
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(subsurfaceScatteringTexture)));

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle(ShaderID::SubsurfaceScatteringRecombine);
                    commandList.DrawIndirect(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderArguments,
                        RenderBackendBufferHandle::Null,
                        registry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer),
                        drawIndirectArgumentBufferOffset,
                        1,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });

        renderGraph.AddPass(std::format("SubsurfaceScatteringCopyResults (Graphics, Tiled)"), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);
                builder.ReadBuffer(tileDataBuffer, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(subsurfaceScatteringTexture, RenderBackendResourceState::ShaderResource);

                auto sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindBuffer(1, registry.GetRenderBackendBufferHandle(tileDataBuffer), 0);
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(subsurfaceScatteringTexture)));

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle(ShaderID::SubsurfaceScatteringCopyResults);
                    commandList.DrawIndirect(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderArguments,
                        RenderBackendBufferHandle::Null,
                        registry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer),
                        drawIndirectArgumentBufferOffset,
                        1,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }
}