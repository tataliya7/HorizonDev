#include "GPUAcceleratedFastFourierTransform.h"

namespace Horizon::GPUFFT
{
    void DispatchSharedMemoryTwoForOneRealFFTCS(
        ShaderCollection* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect)
    {
        if (true) // N < 4096
        {
            const uint32 numGroups = isHorizontal ? srcRect.height : srcRect.width;

            renderGraph.AddPass(
                "FFT",
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    srcTexture = builder.ReadTexture(srcTexture, RenderBackendResourceState::ShaderResource);
                    dstTexture = builder.WriteTexture(dstTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendPushConstantValues shaderConstants = {};
                        shaderConstants.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(srcTexture));
                        shaderConstants.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(dstTexture, 0));

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SharedMemoryTwoForOneRealFFT);
                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
                            1,
                            numGroups,
                            1);
                    };
                });
        }
    }

    void DispatchSharedMemoryComplexFFTCS(
        ShaderCollection* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect)
    {
        if (true) // N < 4096
        {
            const uint32 numGroups = 1024;//isHorizontal ? srcRect.GetHeight() : srcRect.GetWidth();

            renderGraph.AddPass(
                "FFT",
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    srcTexture = builder.ReadTexture(srcTexture, RenderBackendResourceState::ShaderResource);
                    dstTexture = builder.WriteTexture(dstTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendPushConstantValues shaderConstants = {};
                        shaderConstants.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(srcTexture));
                        shaderConstants.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(dstTexture, 0));

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SharedMemoryComplexFFT);
                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
                            1,
                            numGroups,
                            1);
                    };
                });
        }
    }

    void DispatchSharedMemoryComplexIFFTCS(
        ShaderCollection* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect)
    {
        if (true) // N < 4096
        {
            const uint32 numGroups = 1024;//isHorizontal ? srcRect.GetHeight() : srcRect.GetWidth();

            renderGraph.AddPass(
                "FFT",
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    srcTexture = builder.ReadTexture(srcTexture, RenderBackendResourceState::ShaderResource);
                    dstTexture = builder.WriteTexture(dstTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendPushConstantValues shaderConstants = {};
                        shaderConstants.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(srcTexture));
                        shaderConstants.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(dstTexture, 0));

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SharedMemoryComplexIFFT);
                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
                            1,
                            numGroups,
                            1);
                    };
                });
        }
    }

    void DispatchSharedMemoryFFTConvolutionCS(
        ShaderCollection* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle kernelTexture,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect)
    {
        const uint32 numGroups = 1024;//isHorizontal ? srcRect.GetHeight() : srcRect.GetWidth();

        renderGraph.AddPass(
            "SharedMemoryFFTConvolution",
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                srcTexture = builder.ReadTexture(srcTexture, RenderBackendResourceState::ShaderResource);
                dstTexture = builder.WriteTexture(dstTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(srcTexture));
                    shaderConstants.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(dstTexture, 0));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(kernelTexture));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SharedMemoryComplexFFTConvolution);
                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        1,
                        numGroups,
                        1);
                };
            });
    }

    void DispatchSharedMemoryTwoForOneRealIFFTCS(
        ShaderCollection* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect)
    {
        if (true) // N < 4096
        {
            const uint32 numGroups = isHorizontal ? srcRect.height : srcRect.width;

            renderGraph.AddPass(
                "IFFT",
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    srcTexture = builder.ReadTexture(srcTexture, RenderBackendResourceState::ShaderResource);
                    dstTexture = builder.WriteTexture(dstTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendPushConstantValues shaderConstants = {};
                        shaderConstants.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(srcTexture));
                        shaderConstants.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(dstTexture, 0));

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SharedMemoryTwoForOneRealIFFT);
                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
                            1,
                            numGroups,
                            1);
                    };
                });
        }
    }
}