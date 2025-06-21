#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    // TODO: may we don't need to get a shader library
    class ShaderCollection;

    class RendererDefaultResources
    {
    public:

        RendererDefaultResources(RenderBackend* renderBackend, RenderGraphResourcePool* resourcePool, ShaderCollection* shaderLibrary);

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
        ShaderCollection* shaderLibrary;
        bool initialized;

        RenderBackendTextureHandle environmentBrdfLutTexture;

        RenderBackendSamplerHandle globalSamplerLinearWarp;
        RenderBackendSamplerHandle globalSamplerLinearClamp;
        RenderBackendSamplerHandle globalSamplerLinearBorder;
        RenderBackendSamplerHandle globalSamplerLinearMirror;
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

    static inline uint32 ComputeShaderThreadGroupCount(uint32 threads, uint32 threadsPerGroup)
    {
        return ((threads + threadsPerGroup - 1) / threadsPerGroup);
    }

    static inline Extent2D DownsampleExtent2D(Extent2D srcExtent, uint32 downsampleFactor)
    {
        uint32 w = std::max(1u, ((srcExtent.width + downsampleFactor - 1u) / downsampleFactor));
        uint32 h = std::max(1u, ((srcExtent.height + downsampleFactor - 1u) / downsampleFactor));
        return Extent2D(w, h);
    }

    static inline Vector4f GetSizeAndInverseSize(uint32 width, uint32 height)
    {
        float fWidth = float(width);
        float fHeight = float(height);
        return Vector4f(fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight);
    };

    class ShaderCollection;

    extern void Texture2DGenerateMips(RenderBackend* renderBackend, ShaderCollection* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle textureHandle, uint32 width, uint32 height, uint32 numMipLevels);
}