#pragma once

#include "RenderBackendCommon.h"

namespace Horizon
{
    enum class RenderBackendTextureFormat
    {
        Unknown,

        // Norm
        R8Unorm,
        R8Snorm,
        R16Unorm,
        R16Snorm,
        R8G8Unorm,
        R8G8Snorm,
        R16G16Unorm,
        R16G16Snorm,
        R16G16B16Unorm,
        R16G16B16Snorm,
        R8G8B8A8Unorm,
        R8G8B8A8Snorm,
        R16G16B16A16Unorm,

        // UnormSrgb
        R8G8B8A8UnormSrgb,

        // Float
        R16Float,
        R16G16Float,
        R16G16B16Float,
        R16G16B16A16Float,
        R32Float,
        R32G32Float,
        R32G32B32Float,
        R32G32B32A32Float,

        // Int
        R8Int,
        R8Uint,
        R16Int,
        R16Uint,
        R32Int,
        R32Uint,
        R8G8Int,
        R8G8Uint,
        R16G16Int,
        R16G16Uint,
        R32G32Int,
        R32G32Uint,
        R16G16B16Int,
        R16G16B16Uint,
        R32G32B32Int,
        R32G32B32Uint,
        R8G8B8A8Int,
        R8G8B8A8Uint,
        R16G16B16A16Int,
        R16G16B16A16Uint,
        R32G32B32A32Int,
        R32G32B32A32Uint,

        // RGB
        R11G11B10Float,

        // BGRA
        B8G8R8A8Unorm,
        B8G8R8A8UnormSrgb,

        // Depth stencil
        D32Float,
        D32FloatS8Uint,
        D16Unorm,
        D24UnormS8Uint,

        R10G10B10A2Unorm,

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