#pragma once

#include "Renderer.h"

namespace HE::GPUFFT
{
    void DispatchSharedMemoryTwoForOneRealFFTCS(
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);

    void DispatchSharedMemoryTwoForOneRealIFFTCS(
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);

    void DispatchSharedMemoryComplexFFTCS(
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);

    void DispatchSharedMemoryComplexIFFTCS(
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);

    void DispatchSharedMemoryFFTConvolutionCS(
        RenderGraph& renderGraph,
        bool isHorizontal,
        uint32 signalLength,
        RenderGraphTextureHandle kernelTexture,
        RenderGraphTextureHandle srcTexture, const Rect& srcRect,
        RenderGraphTextureHandle dstTexture, const Rect& dstRect);
}