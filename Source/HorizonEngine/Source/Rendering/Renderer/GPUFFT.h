#pragma once

#include "RendererCommon.h"

namespace Horizon::GPUFFT
{
    void DispatchSharedMemoryTwoForOneRealFFTCS(
        ShaderLibrary_DEPRECATED* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);

    void DispatchSharedMemoryTwoForOneRealIFFTCS(
        ShaderLibrary_DEPRECATED* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);

    void DispatchSharedMemoryComplexFFTCS(
        ShaderLibrary_DEPRECATED* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);

    void DispatchSharedMemoryComplexIFFTCS(
        ShaderLibrary_DEPRECATED* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);

    void DispatchSharedMemoryFFTConvolutionCS(
        ShaderLibrary_DEPRECATED* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle kernelTexture,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);
}