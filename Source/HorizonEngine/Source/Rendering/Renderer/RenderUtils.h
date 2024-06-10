#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    // TODO: may we don't need to get a shader library
    class ShaderLibrary;

    class RendererDefaultResources
    {
    public:

        RendererDefaultResources(RenderBackend* renderBackend, RenderGraphResourcePool* resourcePool, ShaderLibrary* shaderLibrary);

        ~RendererDefaultResources();

        void Initialize(RenderBackendCommandList& commandList);

        void Release();

        RenderBackendTextureHandle GetPreIntegratedBrdfLut() const;

        RenderGraphTextureHandle ImportBlackDummyTexture2D(RenderGraph& renderGraph) const;

        RenderGraphTextureHandle ImportWhiteDummyTexture2D(RenderGraph& renderGraph) const;

        RenderGraphPersistentTexture* GetBlackDummyTexture2D() const;

        RenderGraphPersistentTexture* GetWhiteDummyTexture2D() const;

    private:

        RenderBackend* renderBackend;
        RenderGraphResourcePool* renderGraphResourcePool;
        ShaderLibrary* shaderLibrary;
        bool initialized;

        RenderBackendTextureHandle preIntegratedBrdfLut;

        RenderBackendSamplerHandle globalSamplerLinearWarp;
        RenderBackendSamplerHandle globalSamplerLinearClamp;
        RenderBackendSamplerHandle globalSamplerLinearBorder;
        RenderBackendSamplerHandle globalSamplerPointWarp;
        RenderBackendSamplerHandle globalSamplerPointClamp;
        RenderBackendSamplerHandle globalSamplerPointBorder;
        RenderBackendSamplerHandle globalSamplerComparisonGreaterLinearClamp;
        RenderBackendSamplerHandle globalSamplerComparisonLessLinearClamp;

        RenderGraphPersistentTexture* blackDummyTexture2D;
        RenderGraphPersistentTexture* whiteDummyTexture2D;
    };

    static inline uint32 ComputeThreadGroupCount(uint32 threadCount, uint32 threadGroupSize)
    {
        return ((threadCount + threadGroupSize - 1) / threadGroupSize);
    }

    static inline Extent2D ComputeDownsampledExtent2D(Extent2D srcExtent, uint32 downsampleFactor)
    {
        uint32 w = std::max(1u, ((srcExtent.width + downsampleFactor - 1u) / downsampleFactor));
        uint32 h = std::max(1u, ((srcExtent.height + downsampleFactor - 1u) / downsampleFactor));
        return Extent2D(w, h);
    }

    static inline Vector4 GetSizeAndInverseSize(uint32 width, uint32 height)
    {
        float fWidth = float(width);
        float fHeight = float(height);
        return Vector4(fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight);
    };

    class ShaderLibrary;

    extern void Texture2DGenerateMips(ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle textureHandle, uint32 width, uint32 height, uint32 numMipLevels);
}