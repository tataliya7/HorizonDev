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

        RenderBackendTextureHandle GetEnvironmentBrdfLutTexture() const;

        RenderGraphTextureHandle ImportBlackDummyTexture2D(RenderGraph& renderGraph) const;

        RenderGraphTextureHandle ImportWhiteDummyTexture2D(RenderGraph& renderGraph) const;

        RenderGraphPersistentTexture* GetBlackDummyTexture2D() const;

        RenderGraphPersistentTexture* GetWhiteDummyTexture2D() const;

    private:

        RenderBackend* renderBackend;
        RenderGraphResourcePool* renderGraphResourcePool;
        ShaderLibrary* shaderLibrary;
        bool initialized;

        RenderBackendTextureHandle environmentBrdfLutTexture;

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

    static inline uint32 asuint(float x)
    {
#if !_HAS_CXX20
        uint32 ret = {};
        memcpy(&ret, &x, sizeof(x));
        static_assert(sizeof(x) == sizeof(ret));
#else
        uint32 ret = std::bit_cast<uint32>(x);
#endif
        return ret;
    }

    static inline float asfloat(uint32 x)
    {
#if !_HAS_CXX20
        uint32 ret = {};
        memcpy(&ret, &x, sizeof(x));
        static_assert(sizeof(ret) == sizeof(x));
#else
        float ret = std::bit_cast<float>(x);
#endif
        return ret;
    }

    static inline uint32 CeilDiv(uint32 x, uint32 d)
    {
        return ((x + d - 1) / d);
    }

    static inline Extent2D DownsampleExtent2D(Extent2D srcExtent, uint32 downsampleFactor)
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

    extern void Texture2DGenerateMips(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle textureHandle, uint32 width, uint32 height, uint32 numMipLevels);
}