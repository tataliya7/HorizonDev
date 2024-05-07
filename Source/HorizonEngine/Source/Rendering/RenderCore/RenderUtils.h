#pragma once

#include "Foundation/FoundationModule.h"

namespace Horizon
{
    static inline uint32 ComputeWorkGroupCount(uint32 x, uint32 y)
    {
        return ((x + y - 1) / y);
    }

    static inline Vector4 GetSizeAndInverseSize(uint32 width, uint32 height)
    {
        float fWidth = float(width);
        float fHeight = float(height);
        return Vector4(fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight);
    };

    static inline Extent2D ComputeDownsampledExtent2D(Extent2D srcExtent, uint32 downsampleFactor)
    {
        uint32 w = std::max(1u, ((srcExtent.width + downsampleFactor - 1u) / downsampleFactor));
        uint32 h = std::max(1u, ((srcExtent.height + downsampleFactor - 1u) / downsampleFactor));
        return Extent2D(w, h);
    }

    RenderBackendTextureHandle LoadTextureFromHDRFile(RenderBackend* renderBackend, const char* filename, RenderBackendTextureDesc* outDesc = nullptr);
    RenderBackendTextureHandle LoadTextureFromFile(RenderBackend* renderBackend, const char* filename, bool autoMipmaps = true, bool filpY = true, RenderBackendTextureFormat format = RenderBackendTextureFormat::BGRA8Unorm);
}