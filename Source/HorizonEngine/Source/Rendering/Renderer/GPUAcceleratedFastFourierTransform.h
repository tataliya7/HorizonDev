#pragma once

#include "RendererCommon.h"
#include "ShaderLibrary.h"

namespace Horizon::GPUFFT
{
    void DispatchSharedMemoryTwoForOneRealFFTCS(
        ShaderLibrary_Deprecated* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);

    void DispatchSharedMemoryTwoForOneRealIFFTCS(
        ShaderLibrary_Deprecated* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);

    void DispatchSharedMemoryComplexFFTCS(
        ShaderLibrary_Deprecated* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);

    void DispatchSharedMemoryComplexIFFTCS(
        ShaderLibrary_Deprecated* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);

    void DispatchSharedMemoryFFTConvolutionCS(
        ShaderLibrary_Deprecated* shaderLibrary,
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle kernelTexture,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);
}