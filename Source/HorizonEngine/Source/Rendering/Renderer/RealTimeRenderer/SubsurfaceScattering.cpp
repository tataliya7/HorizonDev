#include "RealTimeRenderer.h"

namespace Horizon
{
    static constexpr uint32 GSubsurfaceScatteringTileSize = 8;

    bool RealTimeRenderer::IsSubsurfaceScatteringEnabled() const
    {
        return features.enableSubsurfaceScattering;
    }

    void RealTimeRenderer::RenderSubsurfaceScattering(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        const RenderGraphTextureDesc& sceneColorTextureDesc = renderGraph.GetTextureDesc(sceneTextures.sceneColorTexture);

        assert((renderResolution.width == sceneColorTextureDesc.width) && (renderResolution.height == sceneColorTextureDesc.height));

        const uint32 tileCountX = ComputeThreadGroupCount(sceneColorTextureDesc.width, GSubsurfaceScatteringTileSize);
        const uint32 tileCountY = ComputeThreadGroupCount(sceneColorTextureDesc.height, GSubsurfaceScatteringTileSize);
        const uint32 tileCount = tileCountX * tileCountY;

        RenderGraphTextureHandle subsurfaceScatteringTexture = renderGraph.CreateTexture(sceneColorTextureDesc, "SubsurfaceScatteringTexture");

        RenderGraphBufferHandle tileCountBuffer = renderGraph.CreateBuffer(RenderGraphBufferDesc::CreateByteAddress(sizeof(uint32)), "SubsurfaceScatteringTileCountBuffer");
        RenderGraphBufferHandle tileDataBuffer = renderGraph.CreateBuffer(RenderGraphBufferDesc::CreateByteAddress(sizeof(uint32) * 2 * tileCount), "SubsurfaceScatteringTileDataBuffer");

        RenderGraphBufferHandle drawIndirectArgumentBuffer = renderGraph.CreateBuffer(RenderGraphBufferDesc::CreateIndirectArguments(sizeof(RenderBackendDrawIndirectArguments), 1), "SubsurfaceScatteringDrawIndirectArgumentBuffer");
        RenderGraphBufferHandle dispatchIndirectArgumentBuffer = renderGraph.CreateBuffer(RenderGraphBufferDesc::CreateIndirectArguments(sizeof(RenderBackendDispatchIndirectArguments), 1), "SubsurfaceScatteringDispatchIndirectArgumentBuffer");

        renderGraph.AddPass(
            std::format("SubsurfaceScatteringInitialize (Compute, {}x{})", 0, 0),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                tileCountBuffer = builder.WriteBuffer(tileCountBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, registry.GetRenderBackendBufferHandle(tileCountBuffer));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringInitialize);

                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        1,
                        1,
                        1);
                };
            });

        renderGraph.AddPass(
            std::format("SubsurfaceScatteringClassifyTiles (Compute, {}x{})", sceneColorTextureDesc.width, sceneColorTextureDesc.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneColorTexture = builder.ReadTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                tileCountBuffer = builder.WriteBuffer(tileCountBuffer, RenderBackendResourceState::UnorderedAccess);
                tileDataBuffer = builder.WriteBuffer(tileDataBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeThreadGroupCount(sceneColorTextureDesc.width, GSubsurfaceScatteringTileSize);
                    uint32 threadGroupCountY = ComputeThreadGroupCount(sceneColorTextureDesc.height, GSubsurfaceScatteringTileSize);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBufferCBV(0, this->GetCurrentPerFrameConstantBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                    shaderArguments.BindBuffer(3, registry.GetRenderBackendBufferHandle(tileCountBuffer));
                    shaderArguments.BindBuffer(4, registry.GetRenderBackendBufferHandle(tileDataBuffer));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringClassifyTiles);

                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("SubsurfaceScatteringBuildIndirectArguments (Compute, 1x1x1)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                tileCountBuffer = builder.ReadBuffer(tileCountBuffer, RenderBackendResourceState::ShaderResource);
                drawIndirectArgumentBuffer = builder.WriteBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);
                dispatchIndirectArgumentBuffer = builder.WriteBuffer(dispatchIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, registry.GetRenderBackendBufferHandle(tileCountBuffer));
                    shaderArguments.BindBuffer(1, registry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer));
                    shaderArguments.BindBuffer(2, registry.GetRenderBackendBufferHandle(dispatchIndirectArgumentBuffer));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringBuildIndirectArguments);

                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        1,
                        1,
                        1);
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

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringSampleDiffusionProfile);
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

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringComputeVariance);
                    commandList.DispatchIndirect(
                        computeShader,
                        shaderArguments,
                        registry.GetRenderBackendBufferHandle(dispatchIndirectArgumentBuffer),
                        0);
                };
            });
#endif

        renderGraph.AddPass(
            std::format("SubsurfaceScatteringRecombine (Graphics, Tiled)"),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);
                builder.ReadBuffer(tileDataBuffer, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(subsurfaceScatteringTexture, RenderBackendResourceState::ShaderResource);

                auto sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindRenderTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Discard, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBufferCBV(0, this->GetCurrentPerFrameConstantBuffer());
                    shaderArguments.BindBuffer(1, registry.GetRenderBackendBufferHandle(tileDataBuffer));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(subsurfaceScatteringTexture)));

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringRecombineVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringRecombinePS);

                    commandList.DrawIndirect(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderArguments,
                        RenderBackendBufferHandle::Null,
                        registry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer),
                        0,
                        1,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });

        renderGraph.AddPass(
            std::format("SubsurfaceScatteringCopyResults (Graphics, Tiled, {}x{})", sceneColorTextureDesc.width, sceneColorTextureDesc.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                drawIndirectArgumentBuffer = builder.ReadBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);
                tileDataBuffer = builder.ReadBuffer(tileDataBuffer, RenderBackendResourceState::ShaderResource);
                subsurfaceScatteringTexture = builder.ReadTexture(subsurfaceScatteringTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindRenderTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBufferCBV(0, this->GetCurrentPerFrameConstantBuffer());
                    shaderArguments.BindBuffer(1, registry.GetRenderBackendBufferHandle(tileDataBuffer));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(subsurfaceScatteringTexture)));

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringCopyResultsVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringCopyResultsPS);

                    commandList.DrawIndirect(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderArguments,
                        RenderBackendBufferHandle::Null,
                        registry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer),
                        0,
                        1,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }
}