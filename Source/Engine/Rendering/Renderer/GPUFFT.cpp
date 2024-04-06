#include "GPUFFT.h"

namespace HE::GPUFFT
{
    void DispatchSharedMemoryTwoForOneRealFFTCS(
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
                        shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(srcTexture)));
                        shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(dstTexture), 0));

                        auto fftCS = GRenderer->GetShaderLibrary()->GetShaderHandle((uint32)ShaderPipelineID::SharedMemoryTwoForOneRealFFT);
                        commandList.Dispatch2D(
                            fftCS,
                            shaderArguments,
                            1,
                            numGroups);
                    };
                });

        }
    }

    void DispatchSharedMemoryComplexFFTCS(
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
                        shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(srcTexture)));
                        shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(dstTexture), 0));

                        auto complexFFTCS = GRenderer->GetShaderLibrary()->GetShaderHandle((uint32)ShaderPipelineID::SharedMemoryComplexFFT);
                        commandList.Dispatch2D(
                            complexFFTCS,
                            shaderArguments,
                            1,
                            numGroups);
                    };
                });

        }
    }

    void DispatchSharedMemoryComplexIFFTCS(
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
                        shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(srcTexture)));
                        shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(dstTexture), 0));

                        auto complexFFTCS = GRenderer->GetShaderLibrary()->GetShaderHandle((uint32)ShaderPipelineID::SharedMemoryComplexIFFT);
                        commandList.Dispatch2D(
                            complexFFTCS,
                            shaderArguments,
                            1,
                            numGroups);
                    };
                });

        }
    }

    void DispatchSharedMemoryFFTConvolutionCS(
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
                    shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(srcTexture)));
                    shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(dstTexture), 0));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(kernelTexture)));

                    auto convolutionCS = GRenderer->GetShaderLibrary()->GetShaderHandle((uint32)ShaderPipelineID::SharedMemoryComplexFFTConvolution);
                    commandList.Dispatch2D(
                        convolutionCS,
                        shaderArguments,
                        1,
                        numGroups);
                };
            });
    }

    void DispatchSharedMemoryTwoForOneRealIFFTCS(
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
                        shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(srcTexture)));
                        shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(dstTexture), 0));

                        auto ifftCS = GRenderer->GetShaderLibrary()->GetShaderHandle((uint32)ShaderPipelineID::SharedMemoryTwoForOneRealIFFT);
                        commandList.Dispatch2D(
                            ifftCS,
                            shaderArguments,
                            1,
                            numGroups);
                    };
                });

        }
    }
}