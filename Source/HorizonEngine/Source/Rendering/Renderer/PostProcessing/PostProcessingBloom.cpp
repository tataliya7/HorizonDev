#include "PostProcessingPipeline.h"

namespace Horizon
{
    RenderGraphTextureHandle PostProcessingPipeline::DispatchGaussianBloom(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle colorTexture,
        const PostProcessingColorPyramid& colorPyramid)
    {
        uint32 mip0Width = targetResolution.width / 2;
        uint32 mip0Height = targetResolution.height / 2;

        uint32 passCount = std::min(6u, Math::MaxMipLevelCount(mip0Width, mip0Height));
        if (passCount < 1)
        {
            return RenderGraphTextureHandle::Null;
        }

        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "GaussianBloom");

        RenderGraphTextureHandle halfResolutionSceneColorTexture = colorPyramid.textures[0];
        std::vector<RenderGraphTextureHandle> downsampleMipChain;
        downsampleMipChain.push_back(halfResolutionSceneColorTexture);

        {
            RenderGraphTextureHandle inputTexture = halfResolutionSceneColorTexture;
            for (uint32 passIndex = 0; passIndex < passCount; passIndex++)
            {
                bool useKarisAverage = (passIndex == 0) ? true : false;
                uint32 outputTextureWidth = mip0Width >> (1 + passIndex);
                uint32 outputTextureHeight = mip0Height >> (1 + passIndex);

                RenderGraphTextureDescription outputTextureDesc = RenderGraphTextureDescription::Create2D(
                    outputTextureWidth,
                    outputTextureHeight,
                    RenderBackendTextureFormat::R11G11B10Float,
                    RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
                RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "GaussianBloomDownsampleTexture");

                renderGraph.AddPass(
                    std::format("GaussianBloomDownsample (Compute, {}x{})", outputTextureWidth, outputTextureHeight),
                    RenderGraphPassFlags::Compute,
                    [&](RenderGraphBuilder& builder)
                    {
                        inputTexture = builder.ReadTexture(inputTexture, RenderBackendResourceState::ShaderResource);
                        outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                        return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                        {
                            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(outputTextureWidth, PostProcessingThreadGroupSizeX);
                            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(outputTextureHeight, PostProcessingThreadGroupSizeY);
                            uint32 threadGroupCountZ = 1;

                            RenderBackendPushConstantValues pushConstantValues = {};
                            pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                            pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(inputTexture));
                            pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));
                            pushConstantValues.OverrideShaderConstantValue(3, 1.0f / float(outputTextureWidth));
                            pushConstantValues.OverrideShaderConstantValue(4, 1.0f / float(outputTextureHeight));
                            pushConstantValues.OverrideShaderConstantValue(5, useKarisAverage ? 1 : 0);

                            RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::GaussianBloomDownsample);

                            commandList.Dispatch(
                                computeShader,
                                pushConstantValues,
                                threadGroupCountX,
                                threadGroupCountY,
                                threadGroupCountZ);
                        };
                    });

                inputTexture = outputTexture;
                downsampleMipChain.push_back(outputTexture);
            }
        }

        RenderGraphTextureHandle bloomTexture = RenderGraphTextureHandle::Null;
        {
            RenderGraphTextureHandle lowResolutionInputTexture = downsampleMipChain[passCount];
            for (uint32 passIndex = 0; passIndex < passCount; passIndex++)
            {
                RenderGraphTextureHandle downsampledInputTexture = downsampleMipChain[passCount - 1 - passIndex];

                uint32 outputTextureWidth = mip0Width >> (passCount - passIndex - 1);
                uint32 outputTextureHeight = mip0Height >> (passCount - passIndex - 1);

                RenderGraphTextureDescription outputTextureDesc = RenderGraphTextureDescription::Create2D(
                    outputTextureWidth,
                    outputTextureHeight,
                    RenderBackendTextureFormat::R11G11B10Float,
                    RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
                RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "GaussianBloomUpsampleTexture");

                renderGraph.AddPass(
                    std::format("GaussianBloomUpsample (Compute, {}x{})", outputTextureWidth, outputTextureHeight),
                    RenderGraphPassFlags::Compute,
                    [&](RenderGraphBuilder& builder)
                    {
                        builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                        builder.SetBindlessResourceSRV(1, downsampledInputTexture);
                        builder.SetBindlessResourceSRV(2, lowResolutionInputTexture);
                        builder.SetBindlessResourceUAV(3, outputTexture, 0);
                        builder.SetShaderConstantValue(4, 1.0f / static_cast<float>(outputTextureWidth));
                        builder.SetShaderConstantValue(5, 1.0f / static_cast<float>(outputTextureHeight));

                        RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::GaussianBloomUpsample);

                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(outputTextureWidth, PostProcessingThreadGroupSizeX);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(outputTextureHeight, PostProcessingThreadGroupSizeY);
                        uint32 threadGroupCountZ = 1;

                        return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                        {
                            RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                            commandList.ClearTextureUAV(RenderBackendTextureUAVDesc(resourceRegistry.GetRenderBackendTextureHandle(outputTexture), 0), RenderBackendTextureClearValue::Black);

                            commandList.Dispatch(
                                computeShader,
                                pushConstantValues,
                                threadGroupCountX,
                                threadGroupCountY,
                                threadGroupCountZ);
                        };
                    });

                lowResolutionInputTexture = outputTexture;
                bloomTexture = outputTexture;
            }
        }

        return bloomTexture;
    }

    RenderGraphTextureHandle PostProcessingPipeline::DispatchConvolutionBloom(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle colorTexture,
        const PostProcessingColorPyramid& colorPyramid)
    {
        return RenderGraphTextureHandle::Null;
    }
}
