#include "GPUAcceleratedFastFourierTransform.h"
#include "ShaderID.h"

namespace Horizon::GPUFFT
{
    void DispatchSharedMemoryTwoForOneRealFFTCS(
        ShaderLibrary_Deprecated* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect)
    {
        if (true) // N < 4096
        {
            const uint32 numGroups = isHorizontal ? srcRect.GetHeight() : srcRect.GetWidth();

            renderGraph.AddPass("FFT", RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    srcTexture = builder.ReadTexture(srcTexture, RenderBackendResourceState::ShaderResource);
                    dstTexture = builder.WriteTexture(dstTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                        {
                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(srcTexture)));
                            shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(dstTexture), 0));

                            RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::SharedMemoryTwoForOneRealFFT);
                            commandList.Dispatch(
                                computeShader,
                                shaderArguments,
                                1,
                                numGroups,
                                1);
                        };
                });
        }
    }

    void DispatchSharedMemoryComplexFFTCS(
        ShaderLibrary_Deprecated* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect)
    {
        if (true) // N < 4096
        {
            const uint32 numGroups = 1024;//isHorizontal ? srcRect.GetHeight() : srcRect.GetWidth();

            renderGraph.AddPass("FFT", RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    srcTexture = builder.ReadTexture(srcTexture, RenderBackendResourceState::ShaderResource);
                    dstTexture = builder.WriteTexture(dstTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                        {
                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(srcTexture)));
                            shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(dstTexture), 0));

                            RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::SharedMemoryComplexFFT);
                            commandList.Dispatch(
                                computeShader,
                                shaderArguments,
                                1,
                                numGroups,
                                1);
                        };
                });
        }
    }

    void DispatchSharedMemoryComplexIFFTCS(
        ShaderLibrary_Deprecated* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect)
    {
        if (true) // N < 4096
        {
            const uint32 numGroups = 1024;//isHorizontal ? srcRect.GetHeight() : srcRect.GetWidth();

            renderGraph.AddPass("FFT", RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    srcTexture = builder.ReadTexture(srcTexture, RenderBackendResourceState::ShaderResource);
                    dstTexture = builder.WriteTexture(dstTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                        {
                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(srcTexture)));
                            shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(dstTexture), 0));

                            RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::SharedMemoryComplexIFFT);
                            commandList.Dispatch(
                                computeShader,
                                shaderArguments,
                                1,
                                numGroups,
                                1);
                        };
                });
        }
    }

    void DispatchSharedMemoryFFTConvolutionCS(
        ShaderLibrary_Deprecated* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle kernelTexture,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect)
    {
        const uint32 numGroups = 1024;//isHorizontal ? srcRect.GetHeight() : srcRect.GetWidth();

        renderGraph.AddPass("SharedMemoryFFTConvolution", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                srcTexture = builder.ReadTexture(srcTexture, RenderBackendResourceState::ShaderResource);
                dstTexture = builder.WriteTexture(dstTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(srcTexture)));
                        shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(dstTexture), 0));
                        shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(kernelTexture)));

                        RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::SharedMemoryComplexFFTConvolution);
                        commandList.Dispatch(
                            computeShader,
                            shaderArguments,
                            1,
                            numGroups,
                            1);
                    };
            });
    }

    void DispatchSharedMemoryTwoForOneRealIFFTCS(
        ShaderLibrary_Deprecated* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect)
    {
        if (true) // N < 4096
        {
            const uint32 numGroups = isHorizontal ? srcRect.GetHeight() : srcRect.GetWidth();

            renderGraph.AddPass("IFFT", RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    srcTexture = builder.ReadTexture(srcTexture, RenderBackendResourceState::ShaderResource);
                    dstTexture = builder.WriteTexture(dstTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                        {
                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(srcTexture)));
                            shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(dstTexture), 0));

                            RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::SharedMemoryTwoForOneRealIFFT);
                            commandList.Dispatch(
                                computeShader,
                                shaderArguments,
                                1,
                                numGroups,
                                1);
                        };
                });
        }
    }
}