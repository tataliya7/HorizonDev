#include "RasterizationRenderer.h"

namespace Horizon
{
    static constexpr uint32 SubsurfaceScatteringTileSize = 8;

    void RasterizationRenderer::RenderSubsurfaceScattering(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "SubsurfaceScattering");

        const RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();
        const RenderGraphTextureDescription& colorTextureDescription = renderGraph.GetTextureDesc(intermediateResources.colorTexture);

        assert((renderResolution.width == colorTextureDescription.width) && (renderResolution.height == colorTextureDescription.height));

        const uint32 tileCountX = Math::CeilDiv(colorTextureDescription.width, SubsurfaceScatteringTileSize);
        const uint32 tileCountY = Math::CeilDiv(colorTextureDescription.height, SubsurfaceScatteringTileSize);
        const uint32 tileCount = tileCountX * tileCountY;

        RenderGraphTextureHandle subsurfaceScatteringTexture = renderGraph.CreateTexture(colorTextureDescription, "SubsurfaceScatteringTexture");

        RenderGraphBufferHandle tileCountBuffer = renderGraph.CreateBuffer(RenderGraphBufferDescription::CreateByteAddress(sizeof(uint32)), "SubsurfaceScatteringTileCountBuffer");
        RenderGraphBufferHandle tileDataBuffer = renderGraph.CreateBuffer(RenderGraphBufferDescription::CreateByteAddress(sizeof(uint32) * 2 * tileCount), "SubsurfaceScatteringTileDataBuffer");

        RenderGraphBufferHandle drawIndirectArgumentBuffer = renderGraph.CreateBuffer(RenderGraphBufferDescription::CreateIndirectArguments(sizeof(RenderBackendDrawIndirectArguments), 1), "SubsurfaceScatteringDrawIndirectArgumentBuffer");
        RenderGraphBufferHandle dispatchIndirectArgumentBuffer = renderGraph.CreateBuffer(RenderGraphBufferDescription::CreateIndirectArguments(sizeof(RenderBackendDispatchIndirectArguments), 1), "SubsurfaceScatteringDispatchIndirectArgumentBuffer");

        renderGraph.AddPass(
            std::format("SubsurfaceScatteringInitialization (Compute, 1x1x1)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceUAV(0, tileCountBuffer);

                RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringInitialization);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        1,
                        1,
                        1);
                };
            });

        renderGraph.AddPass(
            std::format("SubsurfaceScatteringTileClassification (Compute, {}x{})", colorTextureDescription.width, colorTextureDescription.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, intermediateResources.colorTexture);
                builder.SetBindlessResourceSRV(2, intermediateResources.depthTexture);
                builder.SetBindlessResourceUAV(3, tileCountBuffer);
                builder.SetBindlessResourceUAV(4, tileDataBuffer);

                RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringTileClassification);

                uint32 threadGroupCountX = ComputeShaderThreadGroupCount(colorTextureDescription.width, SubsurfaceScatteringTileSize);
                uint32 threadGroupCountY = ComputeShaderThreadGroupCount(colorTextureDescription.height, SubsurfaceScatteringTileSize);
                uint32 threadGroupCountZ = 1;

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
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
                builder.SetBindlessResourceSRV(0, tileCountBuffer);
                builder.SetBindlessResourceUAV(1, drawIndirectArgumentBuffer);
                builder.SetBindlessResourceUAV(2, dispatchIndirectArgumentBuffer);

                RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringBuildIndirectArguments);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();
                    
                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
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
                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBuffer(0, resourceRegistry.GetRenderBackendBufferHandle(tileCountBuffer), 0);
                    pushConstantValues.BindBuffer(1, resourceRegistry.GetRenderBackendBufferHandle(argumentBuffer), 0);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringSampleDiffusionProfile);
                    commandList.DispatchIndirect(
                        computeShader,
                        pushConstantValues,
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
                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBuffer(0, resourceRegistry.GetRenderBackendBufferHandle(tileCountBuffer), 0);
                    pushConstantValues.BindBuffer(1, resourceRegistry.GetRenderBackendBufferHandle(argumentBuffer), 0);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SubsurfaceScatteringComputeVariance);
                    commandList.DispatchIndirect(
                        computeShader,
                        pushConstantValues,
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
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, tileDataBuffer);
                builder.SetBindlessResourceSRV(2, subsurfaceScatteringTexture);
                builder.ReadBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);

                auto sceneColorTexture = builder.WriteTexture(intermediateResources.colorTexture, RenderBackendResourceState::RenderTarget);

                builder.SetRenderTargetBinding(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Discard, RenderBackendRenderPassStoreOperation::Store);

                RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringLightingCompositionVS);
                RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringLightingCompositionPS);

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

                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();
                    
                    commandList.DrawIndirect(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        pushConstantValues,
                        resourceRegistry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer),
                        0,
                        1,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });

        renderGraph.AddPass(
            std::format("SubsurfaceScatteringCopyResults (Graphics, Tiled, {}x{})", colorTextureDescription.width, colorTextureDescription.height),
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

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(tileDataBuffer));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(subsurfaceScatteringTexture));

                    RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringCopyResultsVS);
                    RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::SubsurfaceScatteringCopyResultsPS);

                    commandList.DrawIndirect(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        pushConstantValues,
                        resourceRegistry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer),
                        0,
                        1,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }
}