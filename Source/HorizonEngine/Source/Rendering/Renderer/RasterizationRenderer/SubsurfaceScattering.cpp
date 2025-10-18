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
        const RenderGraphTextureDescription& colorTextureDescription = renderGraph.GetTextureDescription(intermediateResources.colorTexture);

        assert((renderResolution.width == colorTextureDescription.width) && (renderResolution.height == colorTextureDescription.height));

        const uint32 tileCountX = Math::CeilDiv(colorTextureDescription.width, SubsurfaceScatteringTileSize);
        const uint32 tileCountY = Math::CeilDiv(colorTextureDescription.height, SubsurfaceScatteringTileSize);
        const uint32 tileCount = tileCountX * tileCountY;

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

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SubsurfaceScatteringInitialization);

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

        RenderGraphTextureHandle tileClassificationOutputTexture = renderGraph.CreateTexture(colorTextureDescription, "SubsurfaceScatteringTileClassificationOutputTexture");

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
                builder.SetBindlessResourceUAV(5, tileClassificationOutputTexture, 0);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SubsurfaceScatteringTileClassification);

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

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SubsurfaceScatteringBuildIndirectArguments);

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

        RenderGraphTextureHandle convolutionOutputTexture = renderGraph.CreateTexture(colorTextureDescription, "SubsurfaceScatteringConvolutionOutputTexture");

        renderGraph.AddPass(
            std::format("SubsurfaceScatteringConvolution (Compute, Tiled)"),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, intermediateResources.colorTexture);
                builder.SetBindlessResourceSRV(2, intermediateResources.depthTexture);
                builder.SetBindlessResourceSRV(3, tileDataBuffer);
                builder.SetBindlessResourceUAV(4, convolutionOutputTexture, 0);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SubsurfaceScatteringConvolution);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    RenderBackendBufferHandle argumentBuffer = resourceRegistry.GetRenderBackendBufferHandle(dispatchIndirectArgumentBuffer);

                    commandList.DispatchIndirect(
                        computeShader,
                        pushConstantValues,
                        argumentBuffer,
                        0);
                };
            });

        RenderGraphTextureHandle lightingCompositionOutputTexture = renderGraph.CreateTexture(colorTextureDescription, "SubsurfaceScatteringLightingCompositionOutputTexture");

        renderGraph.AddPass(
            std::format("SubsurfaceScatteringLightingComposition (Graphics, Tiled)"),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);

                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, tileDataBuffer);
                builder.SetBindlessResourceSRV(2, intermediateResources.colorTexture);
                builder.SetBindlessResourceSRV(3, convolutionOutputTexture);

                builder.SetRenderTargetBinding(0, lightingCompositionOutputTexture, RenderBackendRenderPassLoadOperation::Discard, RenderBackendRenderPassStoreOperation::Store);

                RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::SubsurfaceScatteringLightingCompositionVS);
                RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::SubsurfaceScatteringLightingCompositionPS);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, static_cast<float>(colorTextureDescription.width), static_cast<float>(colorTextureDescription.height));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, colorTextureDescription.width, colorTextureDescription.height);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;

                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    RenderBackendBufferHandle argumentBuffer = resourceRegistry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer);

                    commandList.DrawIndirect(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        pushConstantValues,
                        argumentBuffer,
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
                builder.ReadBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);

                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, tileDataBuffer);
                builder.SetBindlessResourceSRV(2, lightingCompositionOutputTexture);

                builder.SetRenderTargetBinding(0, intermediateResources.colorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);

                RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::SubsurfaceScatteringCopyResultsVS);
                RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::SubsurfaceScatteringCopyResultsPS);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, static_cast<float>(colorTextureDescription.width), static_cast<float>(colorTextureDescription.height));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, colorTextureDescription.width, colorTextureDescription.height);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;

                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    RenderBackendBufferHandle argumentBuffer = resourceRegistry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer);

                    commandList.DrawIndirect(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        pushConstantValues,
                        argumentBuffer,
                        0,
                        1,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }
}