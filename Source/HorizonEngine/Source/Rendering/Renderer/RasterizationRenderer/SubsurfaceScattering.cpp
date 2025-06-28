#include "RasterizationRenderer.h"

namespace Horizon
{
    static constexpr uint32 GSubsurfaceScatteringTileSize = 8;

    bool RasterizationRenderer::IsSubsurfaceScatteringEnabled() const
    {
        return renderFeatures.enableSubsurfaceScattering;
    }

    void RasterizationRenderer::RenderSubsurfaceScattering(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "SubsurfaceScattering");

        return;

        const RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();
        const RenderGraphTextureDescription& sceneColorTextureDesc = renderGraph.GetTextureDesc(intermediateResources.colorTexture);

        assert((renderResolution.width == sceneColorTextureDesc.width) && (renderResolution.height == sceneColorTextureDesc.height));

        const uint32 tileCountX = Math::CeilDiv(sceneColorTextureDesc.width, GSubsurfaceScatteringTileSize);
        const uint32 tileCountY = Math::CeilDiv(sceneColorTextureDesc.height, GSubsurfaceScatteringTileSize);
        const uint32 tileCount = tileCountX * tileCountY;

        RenderGraphTextureHandle subsurfaceScatteringTexture = renderGraph.CreateTexture(sceneColorTextureDesc, "SubsurfaceScatteringTexture");

        RenderGraphBufferHandle tileCountBuffer = renderGraph.CreateBuffer(RenderGraphBufferDescription::CreateByteAddress(sizeof(uint32)), "SubsurfaceScatteringTileCountBuffer");
        RenderGraphBufferHandle tileDataBuffer = renderGraph.CreateBuffer(RenderGraphBufferDescription::CreateByteAddress(sizeof(uint32) * 2 * tileCount), "SubsurfaceScatteringTileDataBuffer");

        RenderGraphBufferHandle drawIndirectArgumentBuffer = renderGraph.CreateBuffer(RenderGraphBufferDescription::CreateIndirectArguments(sizeof(RenderBackendDrawIndirectArguments), 1), "SubsurfaceScatteringDrawIndirectArgumentBuffer");
        RenderGraphBufferHandle dispatchIndirectArgumentBuffer = renderGraph.CreateBuffer(RenderGraphBufferDescription::CreateIndirectArguments(sizeof(RenderBackendDispatchIndirectArguments), 1), "SubsurfaceScatteringDispatchIndirectArgumentBuffer");

        renderGraph.AddPass(
            std::format("SubsurfaceScatteringInitialize (Compute, 1x1x1)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                tileCountBuffer = builder.WriteBuffer(tileCountBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferUAV(0, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(tileCountBuffer));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringInitialize);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
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
                RenderGraphTextureHandle sceneColorTexture = builder.ReadTexture(intermediateResources.colorTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::ShaderResource);
                tileCountBuffer = builder.WriteBuffer(tileCountBuffer, RenderBackendResourceState::UnorderedAccess);
                tileDataBuffer = builder.WriteBuffer(tileDataBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(sceneColorTextureDesc.width, GSubsurfaceScatteringTileSize);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(sceneColorTextureDesc.height, GSubsurfaceScatteringTileSize);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneColorTexture));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindBufferUAV(3, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(tileCountBuffer));
                    shaderConstants.BindBufferUAV(4, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(tileDataBuffer));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringClassifyTiles);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
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

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(tileCountBuffer));
                    shaderConstants.BindBufferUAV(1, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(drawIndirectArgumentBuffer));
                    shaderConstants.BindBufferUAV(2, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(dispatchIndirectArgumentBuffer));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringBuildIndirectArguments);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
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


                {
                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBuffer(0, resourceRegistry.GetRenderBackendBufferHandle(tileCountBuffer), 0);
                    shaderConstants.BindBuffer(1, resourceRegistry.GetRenderBackendBufferHandle(argumentBuffer), 0);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringSampleDiffusionProfile);
                    commandList.DispatchIndirect(
                        computeShader,
                        shaderConstants,
                        resourceRegistry.GetRenderBackendBufferHandle(dispatchIndirectArgumentBuffer),
                        0);
                };
            });

        renderGraph.AddPass(std::format("SubsurfaceScatteringComputeVariance (Compute, Tiled)"), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadBuffer(dispatchIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);
                builder.ReadBuffer(tileCountBuffer, RenderBackendResourceState::ShaderResource);

                argumentBuffer = builder.WriteBuffer(argumentBuffer, RenderBackendResourceState::UnorderedAccess);


                {
                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBuffer(0, resourceRegistry.GetRenderBackendBufferHandle(tileCountBuffer), 0);
                    shaderConstants.BindBuffer(1, resourceRegistry.GetRenderBackendBufferHandle(argumentBuffer), 0);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringComputeVariance);
                    commandList.DispatchIndirect(
                        computeShader,
                        shaderConstants,
                        resourceRegistry.GetRenderBackendBufferHandle(dispatchIndirectArgumentBuffer),
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

                auto sceneColorTexture = builder.WriteTexture(intermediateResources.colorTexture, RenderBackendResourceState::RenderTarget);

                builder.SetRenderTargetBinding(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Discard, RenderBackendRenderPassStoreOperation::Store);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, (float)renderResolution.width, (float)renderResolution.height);
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(tileDataBuffer));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(subsurfaceScatteringTexture));

                    RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringRecombineVS);
                    RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringRecombinePS);

                    commandList.DrawIndirect(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderConstants,
                        resourceRegistry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer),
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
                RenderGraphTextureHandle sceneColorTexture = builder.WriteTexture(intermediateResources.colorTexture, RenderBackendResourceState::RenderTarget);

                builder.SetRenderTargetBinding(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(tileDataBuffer));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(subsurfaceScatteringTexture));

                    RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringCopyResultsVS);
                    RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringCopyResultsPS);

                    commandList.DrawIndirect(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderConstants,
                        resourceRegistry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer),
                        0,
                        1,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }
}