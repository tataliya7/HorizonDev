#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    static constexpr uint32 GMotionBlurTileSize = 16;
    static constexpr uint32 GMotionBlurVelocityDilationQuadCountPerInstance = 8;

    bool RealTimeRenderer::IsMotionBlurEnabled() const
    {
        return features.enableMotionBlur;
    }

    RenderGraphTextureHandle RealTimeRenderer::DispatchMotionBlur(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        RenderGraphTextureHandle sceneDepthTexture,
        RenderGraphTextureHandle motionVectorTexture)
    {
        const uint32 tileCountX = Math::CeilDiv(renderResolution.width, GMotionBlurTileSize);
        const uint32 tileCountY = Math::CeilDiv(renderResolution.height, GMotionBlurTileSize);
        const uint32 tileCount = tileCountX * tileCountY;

        RenderGraphTextureDesc velocityAndDepthTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle velocityAndDepthTexture = renderGraph.CreateTexture(velocityAndDepthTextureDesc, "MotionBlurVelocityAndDepthTexture");

        RenderGraphTextureDesc velocityRangeTextureDesc = RenderGraphTextureDesc::Create2D(
            tileCountX,
            tileCountY,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle velocityRangeTexture = renderGraph.CreateTexture(velocityRangeTextureDesc, "MotionBlurVelocityRangeTexture");
        RenderGraphTextureHandle dilatedVelocityRangeTexture = renderGraph.CreateTexture(velocityRangeTextureDesc, "MotionBlurDilatedVelocityRangeTexture");

        renderGraph.AddPass(
            std::format("MotionBlurSetup (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                motionVectorTexture = builder.ReadTexture(motionVectorTexture, RenderBackendResourceState::ShaderResource);
                sceneDepthTexture = builder.ReadTexture(sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                velocityRangeTexture = builder.WriteTexture(velocityRangeTexture, RenderBackendResourceState::UnorderedAccess);
                velocityAndDepthTexture = builder.WriteTexture(velocityAndDepthTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = tileCountX;
                    uint32 threadGroupCountY = tileCountY;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(motionVectorTexture));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndex(velocityRangeTexture, 0));
                    shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndex(velocityAndDepthTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::MotionBlurSetupCS);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        RenderGraphTextureDesc velocityDilationDepthTextureDesc = RenderGraphTextureDesc::Create2D(
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

                builder.BindRenderTarget(0, dilatedVelocityRangeTexture, RenderBackendRenderPassBeginningAccessType::Discard, RenderBackendRenderPassEndingAccessType::Preserve);
                builder.BindDepthStencil(velocityDilationDepthTexture,
                    RenderBackendRenderPassBeginningAccessType::Clear,
                    RenderBackendRenderPassEndingAccessType::Discard,
                    RenderBackendRenderPassBeginningAccessType::NoAccess,
                    RenderBackendRenderPassEndingAccessType::NoAccess,
                    RenderBackendDepthStencilAccessType::DepthWrite_StencilNoAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(tileCountX), float(tileCountY));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, tileCountX, tileCountY);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = true;

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::MotionBlurVelocityDilationScatterVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::MotionBlurVelocityDilationScatterPS);

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

                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(velocityRangeTexture));
                        shaderConstants.BindScalar(2, scatterPassIndex);

                        commandList.Draw(
                            vertexShader,
                            pixelShader,
                            graphicsPipelineState,
                            shaderConstants,
                            vertexCount,
                            instanceCount,
                            0,
                            0,
                            RenderBackendPrimitiveTopology::TriangleList);
                    }
                };
            });

        RenderGraphTextureDesc motionBlurColorTextureDesc = RenderGraphTextureDesc::Create2D(
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
                sceneColorTexture = builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);
                velocityAndDepthTexture = builder.ReadTexture(velocityAndDepthTexture, RenderBackendResourceState::ShaderResource);
                dilatedVelocityRangeTexture = builder.ReadTexture(dilatedVelocityRangeTexture, RenderBackendResourceState::ShaderResource);
                motionBlurColorTexture = builder.WriteTexture(motionBlurColorTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(motionBlurColorTextureDesc.width, GMotionBlurTileSize);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(motionBlurColorTextureDesc.height, GMotionBlurTileSize);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneColorTexture));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(velocityAndDepthTexture));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(dilatedVelocityRangeTexture));
                    shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndex(motionBlurColorTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::MotionBlurReconstructionFilterCS);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return motionBlurColorTexture;
    }
}