#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    bool RealTimeRenderer::IsGaussianBloomEnabled() const
    {
        return features.enableGaussianBloom;
    }

    bool RealTimeRenderer::IsConvolutionBloomEnabled() const
    {
        return features.enableConvolutionBloom;
    }

    RenderGraphTextureHandle RealTimeRenderer::DispatchGaussianBloom(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle halfResolutionSceneColorTexture)
    {
        uint32 mip0Width = targetResolution.width / 2;
        uint32 mip0Height = targetResolution.height / 2;

        uint32 passCount = std::min(6u, Math::MaxMipLevelCount(mip0Width, mip0Height));
        if (passCount < 1)
        {
            // TODO
            return RenderGraphTextureHandle::Null;
        }

        std::vector<RenderGraphTextureHandle> downsampleMipChain;
        downsampleMipChain.push_back(halfResolutionSceneColorTexture);

        // Downsample
        {
            RenderGraphTextureHandle inputTexture = halfResolutionSceneColorTexture;
            for (uint32 passIndex = 0; passIndex < passCount; passIndex++)
            {
                bool useKarisAverage = (passIndex == 0) ? true : false;
                uint32 outputTextureWidth = mip0Width >> (1 + passIndex);
                uint32 outputTextureHeight = mip0Height >> (1 + passIndex);

                RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
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

                        return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                        {
                            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(outputTextureWidth, PostProcessingThreadGroupSizeX);
                            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(outputTextureHeight, PostProcessingThreadGroupSizeY);
                            uint32 threadGroupCountZ = 1;

                            RenderBackendShaderConstants shaderConstants = {};
                            shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                            shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(inputTexture));
                            shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));
                            shaderConstants.BindScalar(3, 1.0f / float(outputTextureWidth));
                            shaderConstants.BindScalar(4, 1.0f / float(outputTextureHeight));
                            shaderConstants.BindScalar(5, useKarisAverage ? 1 : 0);

                            RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::GaussianBloomDownsample);

                            commandList.Dispatch(
                                computeShader,
                                shaderConstants,
                                threadGroupCountX,
                                threadGroupCountY,
                                threadGroupCountZ);
                        };
                    });

                inputTexture = outputTexture;
                downsampleMipChain.push_back(outputTexture);
            }
        }

        // Upsample
        RenderGraphTextureHandle bloomTexture = RenderGraphTextureHandle::Null;
        {
            RenderGraphTextureHandle lowResolutionInputTexture = downsampleMipChain[passCount];
            for (uint32 passIndex = 0; passIndex < passCount; passIndex++)
            {
                RenderGraphTextureHandle downsampledInputTexture = downsampleMipChain[passCount - 1 - passIndex];

                uint32 outputTextureWidth = mip0Width >> (passCount - passIndex - 1);
                uint32 outputTextureHeight = mip0Height >> (passCount - passIndex - 1);

                RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
                    outputTextureWidth,
                    outputTextureHeight,
                    RenderBackendTextureFormat::R11G11B10Float,
                    RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
                RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "GaussianBloomUpsampleTexture");

                // When a resource has the D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET flag, DiscardResource must be called when the discarded subresource regions are in the D3D12_RESOURCE_STATE_RENDER_TARGET resource barrier state.
                renderGraph.AddPass(
                   std::format("DummyPass (Compute, {}x{})", outputTextureWidth, outputTextureHeight),
                   RenderGraphPassFlags::Compute,
                   [&](RenderGraphBuilder& builder)
                   {
                        outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::RenderTarget);

                        return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                        {

                        };
                   });

                renderGraph.AddPass(
                    std::format("GaussianBloomUpsample (Compute, {}x{})", outputTextureWidth, outputTextureHeight),
                    RenderGraphPassFlags::Compute,
                    [&](RenderGraphBuilder& builder)
                    {
                        downsampledInputTexture = builder.ReadTexture(downsampledInputTexture, RenderBackendResourceState::ShaderResource);
                        lowResolutionInputTexture = builder.ReadTexture(lowResolutionInputTexture, RenderBackendResourceState::ShaderResource);
                        outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                        return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                        {
                            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(outputTextureWidth, PostProcessingThreadGroupSizeX);
                            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(outputTextureHeight, PostProcessingThreadGroupSizeY);
                            uint32 threadGroupCountZ = 1;

                            RenderBackendShaderConstants shaderConstants = {};
                            shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                            shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(downsampledInputTexture));
                            shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(lowResolutionInputTexture));
                            shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));
                            shaderConstants.BindScalar(4, 1.0f / float(outputTextureWidth));
                            shaderConstants.BindScalar(5, 1.0f / float(outputTextureHeight));

                            RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::GaussianBloomUpsample);

                            commandList.Dispatch(
                                computeShader,
                                shaderConstants,
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

    RenderGraphTextureHandle RealTimeRenderer::DispatchConvolutionBloom(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture)
    {
        return RenderGraphTextureHandle::Null;
    }
#if 0
        const uint32 downsampleFactor = 2;
        const uint32 sceneColorWidth = view.targetWidth;
        const uint32 sceneColorHeight = view.targetHeight;

        uint32 bloomInputTextureWidth = sceneColorWidth / downsampleFactor;
        uint32 bloomInputTextureHeight = sceneColorHeight / downsampleFactor;
        bloomInputTextureWidth = 1024;
        bloomInputTextureHeight = 1024;

        RenderGraphTextureDesc bloomInputTextureDesc = RenderGraphTextureDesc::Create2D(
            bloomInputTextureWidth,
            bloomInputTextureHeight,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle bloomInputTexture = renderGraph.CreateTexture(bloomInputTextureDesc, "ConvolutionBloomInputTexture");

        bloomInputTexture = AddDownsamplePass(renderGraph, view, sceneColorWidth, sceneColorHeight, bloomInputTextureDesc.width, bloomInputTextureDesc.height, sceneColor, bloomInputTexture);

        RenderGraphTextureDesc bloomOuputTextureDesc = RenderGraphTextureDesc::Create2D(
            bloomInputTextureDesc.width,
            bloomInputTextureDesc.height,
            bloomInputTextureDesc.format,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle bloomOutputTexture = renderGraph.CreateTexture(bloomOuputTextureDesc, "ConvolutionBloomOutputTexture");

        uint32 signalLength = 0;
        bool doHorizontalFirst = true;

        uint32 frequenceSizeX = Math::RoundUpToPowerOfTwo(bloomInputTextureDesc.width);
        uint32 frequenceSizeY = Math::RoundUpToPowerOfTwo(bloomInputTextureDesc.height);

        RenderGraphTextureDesc twoForOneRealFFTTextureDesc = RenderGraphTextureDesc::Create2D(
            2 * (doHorizontalFirst ? frequenceSizeX : bloomInputTextureDesc.width),
            doHorizontalFirst ? bloomInputTextureDesc.height : frequenceSizeY,
            RenderBackendTextureFormat::RGBA32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle twoForOneRealFFTTexture = renderGraph.CreateTexture(twoForOneRealFFTTextureDesc, "TwoForOneRealFFTTexture");
        RenderGraphTextureHandle fftConvolutionTexture = renderGraph.CreateTexture(twoForOneRealFFTTextureDesc, "FFTConvolutionTexture");

        RenderGraphTextureHandle originalBloomKernelTexture = renderGraph.ImportExternalTexture(defaultBloomKernelTexture, defaultBloomKernelTextureDesc, RenderBackendResourceState::ShaderResource, "OriginalBloomKernelTexture");

        RenderGraphTextureDesc bloomKernelTextureDesc = RenderGraphTextureDesc::Create2D(
            bloomInputTextureDesc.width,
            bloomInputTextureDesc.height,
            RenderBackendTextureFormat::RGBA32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle bloomKernelTexture = renderGraph.CreateTexture(bloomKernelTextureDesc, "ConvolutionBloomKernelTexture");
        bloomKernelTexture = AddDownsamplePass(renderGraph, view, defaultBloomKernelTextureDesc.width, defaultBloomKernelTextureDesc.height, bloomInputTextureDesc.width, bloomInputTextureDesc.height, originalBloomKernelTexture, bloomKernelTexture);

        RenderGraphTextureHandle resizedBloomKernelTexture = renderGraph.CreateTexture(bloomKernelTextureDesc, "ResizedBloomKernelTexture");
        renderGraph.AddPass("ConvolutionBloomResizeKernel", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(bloomKernelTexture, RenderBackendResourceState::ShaderResource);
                resizedBloomKernelTexture = builder.WriteTexture(resizedBloomKernelTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeWorkGroupCount(bloomKernelTextureDesc.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeWorkGroupCount(bloomKernelTextureDesc.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindTextureSRV(0, registry.GetTextureSRVBindlessResourceDescriptorIndex(bloomKernelTexture)));
                    shaderConstants.BindTextureUAV(1, registry.GetTextureUAVBindlessResourceDescriptorIndexresizedBloomKernelTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::ConvolutionBloomResizeKernel);
                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        RenderGraphTextureDesc tempTextureDesc = twoForOneRealFFTTextureDesc;
        RenderGraphTextureHandle tempTexture = renderGraph.CreateTexture(tempTextureDesc, "TempTexture");

        RenderGraphTextureDesc tempTexture1Desc = tempTextureDesc;
        RenderGraphTextureHandle tempTexture1 = renderGraph.CreateTexture(tempTexture1Desc, "TempTexture1");

        RenderGraphTextureDesc tempTexture2Desc = bloomKernelTextureDesc;
        RenderGraphTextureHandle tempTexture2 = renderGraph.CreateTexture(tempTexture2Desc, "TempTexture2");

        RenderGraphTextureDesc transformedBloomKernelTextureDesc = twoForOneRealFFTTextureDesc;
        RenderGraphTextureHandle transformedBloomKernelTexture = renderGraph.CreateTexture(transformedBloomKernelTextureDesc, "TransformedBloomKernelTexture");

        GPUFFT::DispatchSharedMemoryTwoForOneRealFFTCS(
            renderGraph,
            true,
            signalLength,
            resizedBloomKernelTexture,
            Rect(0, 0, bloomKernelTextureDesc.width, bloomKernelTextureDesc.height),
            tempTexture,
            Rect(0, 0, tempTextureDesc.width, tempTextureDesc.height));

        GPUFFT::DispatchSharedMemoryComplexFFTCS(
            renderGraph,
            false,
            signalLength,
            tempTexture,
            Rect(0, 0, tempTextureDesc.width, tempTextureDesc.height),
            transformedBloomKernelTexture,
            Rect(0, 0, transformedBloomKernelTextureDesc.width, transformedBloomKernelTextureDesc.height));

        //GPUFFT::DispatchSharedMemoryComplexIFFTCS(
        //    renderGraph,
        //    false,
        //    signalLength,
        //    transformedBloomKernelTexture,
        //    Rect(0, 0, transformedBloomKernelTextureDesc.width, transformedBloomKernelTextureDesc.height),
        //    tempTexture1,
        //    Rect(0, 0, tempTexture1Desc.width, tempTexture1Desc.height));

        //GPUFFT::DispatchSharedMemoryTwoForOneRealIFFTCS(
        //    renderGraph,
        //    true,
        //    signalLength,
        //    tempTexture1,
        //    Rect(0, 0, tempTexture1Desc.width, tempTexture1Desc.height),
        //    tempTexture2,
        //    Rect(0, 0, tempTexture2Desc.width, tempTexture2Desc.height));

        {
            GPUFFT::DispatchSharedMemoryTwoForOneRealFFTCS(
                renderGraph,
                doHorizontalFirst,
                signalLength,
                bloomInputTexture,
                Rect(0, 0, bloomInputTextureDesc.width, bloomInputTextureDesc.height),
                twoForOneRealFFTTexture,
                Rect(0, 0, twoForOneRealFFTTextureDesc.width, twoForOneRealFFTTextureDesc.height));
        }

        {
            GPUFFT::DispatchSharedMemoryFFTConvolutionCS(
                renderGraph,
                !doHorizontalFirst,
                signalLength,
                transformedBloomKernelTexture,
                twoForOneRealFFTTexture,
                Rect(0, 0, twoForOneRealFFTTextureDesc.width, twoForOneRealFFTTextureDesc.height),
                fftConvolutionTexture,
                Rect(0, 0, twoForOneRealFFTTextureDesc.width, twoForOneRealFFTTextureDesc.height));
        }

        {
            GPUFFT::DispatchSharedMemoryTwoForOneRealIFFTCS(
                renderGraph,
                doHorizontalFirst,
                signalLength,
                fftConvolutionTexture,
                Rect(0, 0, twoForOneRealFFTTextureDesc.width, twoForOneRealFFTTextureDesc.height),
                bloomOutputTexture,
                Rect(0, 0, bloomOuputTextureDesc.width, bloomOuputTextureDesc.height));
        }

        return bloomOutputTexture;
    }
#endif
}