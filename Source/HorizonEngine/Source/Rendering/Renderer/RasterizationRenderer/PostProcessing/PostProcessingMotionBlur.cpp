#include "../RasterizationRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    // Must match shader
    static constexpr uint32 MotionBlurTileSize = 16;
    static constexpr uint32 MotionBlurQuadCountPerTile = 8;

    RenderGraphTextureHandle RasterizationRenderer::DispatchMotionBlur(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle colorTexture,
        RenderGraphTextureHandle depthTexture,
        RenderGraphTextureHandle velocityTexture)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "MotionBlur");

        const uint32 tileCountX = Math::CeilDiv(renderResolution.width, MotionBlurTileSize);
        const uint32 tileCountY = Math::CeilDiv(renderResolution.height, MotionBlurTileSize);
        const uint32 tileCount = tileCountX * tileCountY;

        RenderGraphTextureDescription velocityAndDepthTextureDescription = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R32G32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle velocityAndDepthTexture = renderGraph.CreateTexture(velocityAndDepthTextureDescription, "MotionBlurVelocityAndDepthTexture");

        RenderGraphTextureDescription velocityRangeTextureDescription = RenderGraphTextureDescription::Create2D(
            tileCountX,
            tileCountY,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle velocityRangeTexture = renderGraph.CreateTexture(velocityRangeTextureDescription, "MotionBlurVelocityRangeTexture");
        RenderGraphTextureHandle dilatedVelocityRangeTexture = renderGraph.CreateTexture(velocityRangeTextureDescription, "MotionBlurDilatedVelocityRangeTexture");

        // @todo Using a separable approach to calculate the maximum and minimum velocity is faster?
        renderGraph.AddPass(
            std::format("MotionBlurTileClassification (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                depthTexture = builder.ReadTexture(depthTexture, RenderBackendResourceState::ShaderResource);
                velocityTexture = builder.ReadTexture(velocityTexture, RenderBackendResourceState::ShaderResource);
                velocityRangeTexture = builder.WriteTexture(velocityRangeTexture, RenderBackendResourceState::UnorderedAccess);
                velocityAndDepthTexture = builder.WriteTexture(velocityAndDepthTexture, RenderBackendResourceState::UnorderedAccess);

                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, depthTexture);
                builder.SetBindlessResourceSRV(2, velocityTexture);
                builder.SetBindlessResourceUAV(3, velocityRangeTexture, 0);
                builder.SetBindlessResourceUAV(4, velocityAndDepthTexture, 0);
                builder.SetShaderConstantValue(5, renderResolution.width - 1);
                builder.SetShaderConstantValue(6, renderResolution.height - 1);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::MotionBlurTileClassificationCS);

                uint32 threadGroupCountX = tileCountX;
                uint32 threadGroupCountY = tileCountY;
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

        RenderGraphTextureDescription velocityDilationDepthTextureDescription = RenderGraphTextureDescription::Create2D(
            tileCountX,
            tileCountY,
            RenderBackendTextureFormat::D16Unorm,
            RenderBackendTextureCreateFlags::DepthStencil,
            RenderBackendTextureClearValue::CreateDepthValue(1.0f));
        RenderGraphTextureHandle velocityDilationDepthTexture = renderGraph.CreateTexture(velocityDilationDepthTextureDescription, "MotionBlurVelocityDilationDepthTexture");

        renderGraph.AddPass(
            std::format("MotionBlurVelocityDilation (Graphics, {}x{})", tileCountX, tileCountY),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, velocityRangeTexture);

                builder.SetRenderTargetBinding(0, dilatedVelocityRangeTexture, RenderBackendRenderPassLoadOperation::Discard, RenderBackendRenderPassStoreOperation::Store);
                builder.SetDepthStencilBinding(velocityDilationDepthTexture,
                    RenderBackendRenderPassLoadOperation::Clear,
                    RenderBackendRenderPassStoreOperation::Discard,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthWrite_StencilNoAccess);

                RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::MotionBlurVelocityDilationScatterVS);
                RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::MotionBlurVelocityDilationScatterPS);

                uint32 vertexCount = 6 * MotionBlurQuadCountPerTile;
                uint32 instanceCount = Math::CeilDiv(tileCount, MotionBlurQuadCountPerTile);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, static_cast<float>(tileCountX), static_cast<float>(tileCountY));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, tileCountX, tileCountY);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = true;

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

                        RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();
                        pushConstantValues.OverrideShaderConstantValue(2, scatterPassIndex);

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

        RenderGraphTextureDescription motionBlurColorTextureDescription = RenderGraphTextureDescription::Create2D(
            targetResolution.width,
            targetResolution.height,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle motionBlurColorTexture = renderGraph.CreateTexture(motionBlurColorTextureDescription, "MotionBlurColorTexture");

        renderGraph.AddPass(
            std::format("MotionBlurReconstructionFilter (Compute, {}x{})", motionBlurColorTextureDescription.width, motionBlurColorTextureDescription.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, colorTexture);
                builder.SetBindlessResourceSRV(2, dilatedVelocityRangeTexture);
                builder.SetBindlessResourceSRV(3, velocityAndDepthTexture);
                builder.SetBindlessResourceUAV(4, motionBlurColorTexture, 0);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::MotionBlurReconstructionFilterCS);

                uint32 threadGroupCountX = ComputeShaderThreadGroupCount(motionBlurColorTextureDescription.width, MotionBlurTileSize);
                uint32 threadGroupCountY = ComputeShaderThreadGroupCount(motionBlurColorTextureDescription.height, MotionBlurTileSize);
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

        return motionBlurColorTexture;
    }
}