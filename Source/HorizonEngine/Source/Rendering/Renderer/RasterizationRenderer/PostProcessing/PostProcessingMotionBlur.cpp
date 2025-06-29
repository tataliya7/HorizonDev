#include "../RasterizationRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    // Must match shader
    static constexpr uint32 GMotionBlurTileSize = 16;
    static constexpr uint32 GMotionBlurVelocityDilationQuadCountPerInstance = 8;

    RenderGraphTextureHandle RasterizationRenderer::DispatchMotionBlur(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle colorTexture,
        RenderGraphTextureHandle depthTexture,
        RenderGraphTextureHandle velocityTexture)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "MotionBlur");

        const uint32 tileCountX = Math::CeilDiv(renderResolution.width, GMotionBlurTileSize);
        const uint32 tileCountY = Math::CeilDiv(renderResolution.height, GMotionBlurTileSize);
        const uint32 tileCount = tileCountX * tileCountY;

        RenderGraphTextureDescription velocityAndDepthTextureDesc = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R32G32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle velocityAndDepthTexture = renderGraph.CreateTexture(velocityAndDepthTextureDesc, "MotionBlurVelocityAndDepthTexture");

        RenderGraphTextureDescription velocityRangeTextureDesc = RenderGraphTextureDescription::Create2D(
            tileCountX,
            tileCountY,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle velocityRangeTexture = renderGraph.CreateTexture(velocityRangeTextureDesc, "MotionBlurVelocityRangeTexture");
        RenderGraphTextureHandle dilatedVelocityRangeTexture = renderGraph.CreateTexture(velocityRangeTextureDesc, "MotionBlurDilatedVelocityRangeTexture");

        // Using a separable approach to calculate the maximum and minimum velocity is faster?

        renderGraph.AddPass(
            std::format("MotionBlurTileClassification (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                depthTexture = builder.ReadTexture(depthTexture, RenderBackendResourceState::ShaderResource);
                velocityTexture = builder.ReadTexture(velocityTexture, RenderBackendResourceState::ShaderResource);
                velocityRangeTexture = builder.WriteTexture(velocityRangeTexture, RenderBackendResourceState::UnorderedAccess);
                velocityAndDepthTexture = builder.WriteTexture(velocityAndDepthTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = tileCountX;
                    uint32 threadGroupCountY = tileCountY;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(depthTexture));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(velocityTexture));
                    pushConstantValues.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(velocityRangeTexture, 0));
                    pushConstantValues.BindTextureUAV(4, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(velocityAndDepthTexture, 0));
                    pushConstantValues.BindScalar(5, renderResolution.width - 1);
                    pushConstantValues.BindScalar(6, renderResolution.height - 1);

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::MotionBlurTileClassificationCS);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        RenderGraphTextureDescription velocityDilationDepthTextureDesc = RenderGraphTextureDescription::Create2D(
            tileCountX,
            tileCountY,
            RenderBackendTextureFormat::D16Unorm,
            RenderBackendTextureCreateFlags::DepthStencil,
            RenderBackendTextureClearValue::CreateDepthValue(1.0f));
        RenderGraphTextureHandle velocityDilationDepthTexture = renderGraph.CreateTexture(velocityDilationDepthTextureDesc, "MotionBlurVelocityDilationDepthTexture");

        renderGraph.AddPass(
            std::format("MotionBlurVelocityDilation (Graphics, {}x{})", tileCountX, tileCountY),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                velocityRangeTexture = builder.ReadTexture(velocityRangeTexture, RenderBackendResourceState::ShaderResource);
                dilatedVelocityRangeTexture = builder.WriteTexture(dilatedVelocityRangeTexture, RenderBackendResourceState::RenderTarget);
                velocityDilationDepthTexture = builder.WriteTexture(velocityDilationDepthTexture, RenderBackendResourceState::DepthStencil);

                builder.SetRenderTargetBinding(0, dilatedVelocityRangeTexture, RenderBackendRenderPassLoadOperation::Discard, RenderBackendRenderPassStoreOperation::Store);
                builder.SetDepthStencilBinding(velocityDilationDepthTexture,
                    RenderBackendRenderPassLoadOperation::Clear,
                    RenderBackendRenderPassStoreOperation::Discard,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthWrite_StencilNoAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(tileCountX), float(tileCountY));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, tileCountX, tileCountY);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = true;

                    RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::MotionBlurVelocityDilationScatterVS);
                    RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::MotionBlurVelocityDilationScatterPS);

                    uint32 vertexCount = 6 * GMotionBlurVelocityDilationQuadCountPerInstance;
                    uint32 instanceCount = Math::CeilDiv(tileCount, GMotionBlurVelocityDilationQuadCountPerInstance);

                    for (uint32 scatterPassIndex = 0; scatterPassIndex < 2; scatterPassIndex++)
                    {
                        if (scatterPassIndex == 0) // Min
                        {
                            graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::RGBA;
                            graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::Less;
                        }
                        else // Max
                        {
                            graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::B | RenderBackendColorComponentFlags::A;
                            graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::Greater;
                        }

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(velocityRangeTexture));
                        pushConstantValues.BindScalar(2, scatterPassIndex);

                        commandList.Draw(
                            vertexShader,
                            pixelShader,
                            graphicsPipelineState,
                            pushConstantValues,
                            vertexCount,
                            instanceCount,
                            0,
                            0,
                            RenderBackendPrimitiveTopology::TriangleList);
                    }
                };
            });

        RenderGraphTextureDescription motionBlurColorTextureDesc = RenderGraphTextureDescription::Create2D(
            targetResolution.width,
            targetResolution.height,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle motionBlurColorTexture = renderGraph.CreateTexture(motionBlurColorTextureDesc, "MotionBlurColorTexture");

        renderGraph.AddPass(
            std::format("MotionBlurReconstructionFilter (Compute, {}x{})", motionBlurColorTextureDesc.width, motionBlurColorTextureDesc.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                colorTexture = builder.ReadTexture(colorTexture, RenderBackendResourceState::ShaderResource);
                dilatedVelocityRangeTexture = builder.ReadTexture(dilatedVelocityRangeTexture, RenderBackendResourceState::ShaderResource);
                velocityAndDepthTexture = builder.ReadTexture(velocityAndDepthTexture, RenderBackendResourceState::ShaderResource);
                motionBlurColorTexture = builder.WriteTexture(motionBlurColorTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(motionBlurColorTextureDesc.width, GMotionBlurTileSize);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(motionBlurColorTextureDesc.height, GMotionBlurTileSize);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(colorTexture));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(dilatedVelocityRangeTexture));
                    pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(velocityAndDepthTexture));
                    pushConstantValues.BindTextureUAV(4, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(motionBlurColorTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::MotionBlurReconstructionFilterCS);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return motionBlurColorTexture;
    }
}