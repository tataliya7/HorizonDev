#pragma once

#include "RenderBackendCommon.h"

namespace Horizon
{
    enum class RenderBackendTextureFormat : uint32
    {
        Unknown,

        // Norm
        R8Unorm,
        R8Snorm,
        R16Unorm,
        R16Snorm,
        RG8Unorm,
        RG8Snorm,
        RG16Unorm,
        RG16Snorm,
        RGB16Unorm,
        RGB16Snorm,
        RGBA8Unorm,
        RGBA8Snorm,
        RGBA16Unorm,

        // UnormSrgb
        RGBA8UnormSrgb,

        // Float
        R16Float,
        RG16Float,
        RGB16Float,
        RGBA16Float,
        R32Float,
        RG32Float,
        RGB32Float,
        RGBA32Float,

        // Int
        R8Int,
        R8Uint,
        R16Int,
        R16Uint,
        R32Int,
        R32Uint,
        RG8Int,
        RG8Uint,
        RG16Int,
        RG16Uint,
        RG32Int,
        RG32Uint,
        RGB16Int,
        RGB16Uint,
        RGB32Int,
        RGB32Uint,
        RGBA8Int,
        RGBA8Uint,
        RGBA16Int,
        RGBA16Uint,
        RGBA32Int,
        RGBA32Uint,

        // RGB
        R11G11B10Float,

        // BGRA
        BGRA8Unorm,
        BGRA8UnormSrgb,

        // Depth stencil
        D32Float,
        D16Unorm,
        D24UnormS8Uint,

        RGB10A2Unorm,

        // Compressed
        BC1Unorm,
        BC1UnormSrgb,
        BC2Unorm,
        BC2UnormSrgb,
        BC3Unorm,
        BC3UnormSrgb,
        BC4Unorm,
        BC4Snorm,
        BC5Unorm,
        BC5Snorm,
        BC6HUF,
        BC6HSF,
        BC7Unorm,
        BC7UnormSrgb,

        // Count
        Count
    };

    enum class RenderBackendTextureFormatType
    {
        Unknown,        ///< Unknown
        Float,          ///< Floating-point
        Sint,           ///< Signed integer
        Uint,           ///< Unsigned integer
        Snorm,          ///< Signed normalized
        Unorm,          ///< Unsigned normalized
        UnormSrgb,      ///< Unsigned normalized sRGB
    };

    struct RenderBackendTextureFormatDesc
    {
        const char* name;
        RenderBackendTextureFormat format;
        RenderBackendTextureFormatType type;
        uint32 bytes;
        uint32 channels;
        struct
        {
            bool isDepth;
            bool isStencil;
        };
        uint32 channelBits[4];
    };

    extern const RenderBackendTextureFormatDesc& RenderBackendGetTextureFormatDesc(RenderBackendTextureFormat format);
}