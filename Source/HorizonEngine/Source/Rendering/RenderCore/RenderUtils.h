#pragma once

#include "RenderCoreCommon.h"

namespace Horizon
{
    class RendererDefaultResources
    {
    public:
        RenderGraphTextureHandle ImportBlackDummyTexture2D(RenderGraph& renderGraph) const;
        RenderGraphTextureHandle ImportWhiteDummyTexture2D(RenderGraph& renderGraph) const;
        RenderBackendTextureHandle GetPreIntegratedBrdfLut() const;
    private:
        RenderGraphPersistentTexture* blackDummyTexture2D;
        RenderGraphPersistentTexture* whiteDummyTexture2D;

        static const uint32 PreIntegratedBrdfLutSize = 256;
        RenderBackendTextureHandle preIntegratedBrdfLut;

        RenderBackendSamplerHandle globalSamplerLinearWarp;
        RenderBackendSamplerHandle globalSamplerLinearClamp;
        RenderBackendSamplerHandle globalSamplerLinearBorder;
        RenderBackendSamplerHandle globalSamplerPointWarp;
        RenderBackendSamplerHandle globalSamplerPointClamp;
        RenderBackendSamplerHandle globalSamplerPointBorder;
        RenderBackendSamplerHandle globalSamplerComparisonGreaterLinearClamp;
        RenderBackendSamplerHandle globalSamplerComparisonLessLinearClamp;
    };

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

    extern RenderBackendTextureHandle LoadTextureFromHDRFile(RenderBackend* renderBackend, const char* filename, RenderBackendTextureDesc* outDesc = nullptr);
    extern RenderBackendTextureHandle LoadTextureFromFile(RenderBackend* renderBackend, const char* filename, bool autoMipmaps = true, bool filpY = true, RenderBackendTextureFormat format = RenderBackendTextureFormat::BGRA8Unorm);
    extern void Texture2DGenerateMips(ShaderLibrary_DEPRECATED* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle textureHandle, uint32 width, uint32 height, uint32 numMipLevels);
}