#include "RenderBackendTextureFormat.h"

namespace HE
{
    static const RenderBackendTextureFormatDesc GRenderBackendTextureFormatTable[] =
    {
        // name                format                                         type                                          bytes  channels  { depth, stencil }   channelBits // TODO: Remove this
        { "Unknown",           RenderBackendTextureFormat::Unknown,           RenderBackendTextureFormatType::Unknown,      0,     0,        { false, false },    { 0,  0,  0,  0  } },

        { "R8Unorm",           RenderBackendTextureFormat::R8Unorm,           RenderBackendTextureFormatType::Unorm,        1,     1,        { false, false },    { 8,  0,  0,  0  } },
        { "R8Snorm",           RenderBackendTextureFormat::R8Snorm,           RenderBackendTextureFormatType::Snorm,        1,     1,        { false, false },    { 8,  0,  0,  0  } },
        { "R16Unorm",          RenderBackendTextureFormat::R16Unorm,          RenderBackendTextureFormatType::Unorm,        2,     1,        { false, false },    { 16, 0,  0,  0  } },
        { "R16Snorm",          RenderBackendTextureFormat::R16Snorm,          RenderBackendTextureFormatType::Snorm,        2,     1,        { false, false },    { 16, 0,  0,  0  } },
        { "RG8Unorm",          RenderBackendTextureFormat::RG8Unorm,          RenderBackendTextureFormatType::Unorm,        2,     2,        { false, false },    { 8,  8,  0,  0  } },
        { "RG8Snorm",          RenderBackendTextureFormat::RG8Snorm,          RenderBackendTextureFormatType::Snorm,        2,     2,        { false, false },    { 8,  8,  0,  0  } },
        { "RG16Unorm",         RenderBackendTextureFormat::RG16Unorm,         RenderBackendTextureFormatType::Unorm,        4,     2,        { false, false },    { 16, 16, 0,  0  } },
        { "RG16Snorm",         RenderBackendTextureFormat::RG16Snorm,         RenderBackendTextureFormatType::Snorm,        4,     2,        { false, false },    { 16, 16, 0,  0  } },
        { "RGB16Unorm",        RenderBackendTextureFormat::RGB16Unorm,        RenderBackendTextureFormatType::Unorm,        6,     3,        { false, false },    { 16, 16, 16, 0  } },
        { "RGB16Snorm",        RenderBackendTextureFormat::RGB16Snorm,        RenderBackendTextureFormatType::Snorm,        6,     3,        { false, false },    { 16, 16, 16, 0  } },
        { "RGBA8Unorm",        RenderBackendTextureFormat::RGBA8Unorm,        RenderBackendTextureFormatType::Unorm,        4,     4,        { false, false },    { 8,  8,  8,  8  } },
        { "RGBA8Snorm",        RenderBackendTextureFormat::RGBA8Snorm,        RenderBackendTextureFormatType::Snorm,        4,     4,        { false, false },    { 8,  8,  8,  8  } },
        { "RGBA16Unorm",       RenderBackendTextureFormat::RGBA16Unorm,       RenderBackendTextureFormatType::Unorm,        8,     4,        { false, false },    { 16, 16, 16, 16 } },

        { "RGBA8UnormSrgb",    RenderBackendTextureFormat::RGBA8UnormSrgb,    RenderBackendTextureFormatType::UnormSrgb,    4,     4,        { false, false },    { 8,  8,  8,  8  } },

        { "R16Float",          RenderBackendTextureFormat::R16Float,          RenderBackendTextureFormatType::Float,        2,     1,        { false, false },    { 16, 0,  0,  0  } },
        { "RG16Float",         RenderBackendTextureFormat::RG16Float,         RenderBackendTextureFormatType::Float,        4,     2,        { false, false },    { 16, 16, 0,  0  } },
        { "RGB16Float",        RenderBackendTextureFormat::RGB16Float,        RenderBackendTextureFormatType::Float,        6,     3,        { false, false },    { 16, 16, 16, 0  } },
        { "RGBA16Float",       RenderBackendTextureFormat::RGBA16Float,       RenderBackendTextureFormatType::Float,        8,     4,        { false, false },    { 16, 16, 16, 16 } },
        { "R32Float",          RenderBackendTextureFormat::R32Float,          RenderBackendTextureFormatType::Float,        4,     1,        { false, false },    { 32, 0,  0,  0  } },
        { "RG32Float",         RenderBackendTextureFormat::RG32Float,         RenderBackendTextureFormatType::Float,        8,     2,        { false, false },    { 32, 32, 0,  0  } },
        { "RGB32Float",        RenderBackendTextureFormat::RGB32Float,        RenderBackendTextureFormatType::Float,        12,    3,        { false, false },    { 32, 32, 32, 0  } },
        { "RGBA32Float",       RenderBackendTextureFormat::RGBA32Float,       RenderBackendTextureFormatType::Float,        16,    4,        { false, false },    { 32, 32, 32, 32 } },

        { "R8Int",             RenderBackendTextureFormat::R8Int,             RenderBackendTextureFormatType::Sint,         1,     1,        { false, false },    { 8,  0,  0,  0  } },
        { "R8Uint",            RenderBackendTextureFormat::R8Uint,            RenderBackendTextureFormatType::Uint,         1,     1,        { false, false },    { 8,  0,  0,  0  } },
        { "R16Int",            RenderBackendTextureFormat::R16Int,            RenderBackendTextureFormatType::Sint,         2,     1,        { false, false },    { 16, 0,  0,  0  } },
        { "R16Uint",           RenderBackendTextureFormat::R16Uint,           RenderBackendTextureFormatType::Uint,         2,     1,        { false, false },    { 16, 0,  0,  0  } },
        { "R32Int",            RenderBackendTextureFormat::R32Int,            RenderBackendTextureFormatType::Sint,         4,     1,        { false, false },    { 32, 0,  0,  0  } },
        { "R32Uint",           RenderBackendTextureFormat::R32Uint,           RenderBackendTextureFormatType::Uint,         4,     1,        { false, false },    { 32, 0,  0,  0  } },
        { "RG8Int",            RenderBackendTextureFormat::RG8Int,            RenderBackendTextureFormatType::Sint,         2,     2,        { false, false },    { 8,  8,  0,  0  } },
        { "RG8Uint",           RenderBackendTextureFormat::RG8Uint,           RenderBackendTextureFormatType::Uint,         2,     2,        { false, false },    { 8,  8,  0,  0  } },
        { "RG16Int",           RenderBackendTextureFormat::RG16Int,           RenderBackendTextureFormatType::Sint,         4,     2,        { false, false },    { 16, 16, 0,  0  } },
        { "RG16Uint",          RenderBackendTextureFormat::RG16Uint,          RenderBackendTextureFormatType::Uint,         4,     2,        { false, false },    { 16, 16, 0,  0  } },
        { "RG32Int",           RenderBackendTextureFormat::RG32Int,           RenderBackendTextureFormatType::Sint,         8,     2,        { false, false },    { 32, 32, 0,  0  } },
        { "RG32Uint",          RenderBackendTextureFormat::RG32Uint,          RenderBackendTextureFormatType::Uint,         8,     2,        { false, false },    { 32, 32, 0,  0  } },
        { "RGB16Int",          RenderBackendTextureFormat::RGB16Int,          RenderBackendTextureFormatType::Sint,         6,     3,        { false, false },    { 16, 16, 16, 0  } },
        { "RGB16Uint",         RenderBackendTextureFormat::RGB16Uint,         RenderBackendTextureFormatType::Uint,         6,     3,        { false, false },    { 16, 16, 16, 0  } },
        { "RGB32Int",          RenderBackendTextureFormat::RGB32Int,          RenderBackendTextureFormatType::Sint,         12,    3,        { false, false },    { 32, 32, 32, 0  } },
        { "RGB32Uint",         RenderBackendTextureFormat::RGB32Uint,         RenderBackendTextureFormatType::Uint,         12,    3,        { false, false },    { 32, 32, 32, 0  } },
        { "RGBA8Int",          RenderBackendTextureFormat::RGBA8Int,          RenderBackendTextureFormatType::Sint,         4,     4,        { false, false },    { 8,  8,  8,  8  } },
        { "RGBA8Uint",         RenderBackendTextureFormat::RGBA8Uint,         RenderBackendTextureFormatType::Uint,         4,     4,        { false, false },    { 8,  8,  8,  8  } },
        { "RGBA16Int",         RenderBackendTextureFormat::RGBA16Int,         RenderBackendTextureFormatType::Sint,         8,     4,        { false, false },    { 16, 16, 16, 16 } },
        { "RGBA16Uint",        RenderBackendTextureFormat::RGBA16Uint,        RenderBackendTextureFormatType::Uint,         8,     4,        { false, false },    { 16, 16, 16, 16 } },
        { "RGBA32Int",         RenderBackendTextureFormat::RGBA32Int,         RenderBackendTextureFormatType::Sint,         16,    4,        { false, false },    { 32, 32, 32, 32 } },
        { "RGBA32Uint",        RenderBackendTextureFormat::RGBA32Uint,        RenderBackendTextureFormatType::Uint,         16,    4,        { false, false },    { 32, 32, 32, 32 } },
        
        { "B10G11R11Float",    RenderBackendTextureFormat::R11G11B10Float,    RenderBackendTextureFormatType::Float,        4,     3,        { false, false },    { 10, 11, 11, 0  } },

        { "BGRA8Unorm",        RenderBackendTextureFormat::BGRA8Unorm,        RenderBackendTextureFormatType::Unorm,        4,     4,        { false, false },    { 8,  8,  8,  8  } },
        { "BGRA8UnormSrgb",    RenderBackendTextureFormat::BGRA8UnormSrgb,    RenderBackendTextureFormatType::UnormSrgb,    4,     4,        { false, false },    { 8,  8,  8,  8  } },

        { "D32Float",          RenderBackendTextureFormat::D32Float,          RenderBackendTextureFormatType::Float,        4,     1,        { true,  false },    { 32, 0,  0,  0  } },
        { "D16Unorm",          RenderBackendTextureFormat::D16Unorm,          RenderBackendTextureFormatType::Unorm,        2,     1,        { true,  false },    { 16, 0,  0,  0  } },
        { "D24UnormS8Uint",    RenderBackendTextureFormat::D24UnormS8Uint,    RenderBackendTextureFormatType::Unorm,        4,     2,        { true,  true  },    { 24, 8,  0,  0  } },

        { "RGB10A2Unorm",      RenderBackendTextureFormat::RGB10A2Unorm,      RenderBackendTextureFormatType::Unorm,        4,     4,        { false, false },    { 10, 10, 10, 2 } },

        { "BC1Unorm",          RenderBackendTextureFormat::BC1Unorm,          RenderBackendTextureFormatType::Unorm,        8,     3,        { false, false },    { 64, 0,  0,  0  } },
        { "BC1UnormSrgb",      RenderBackendTextureFormat::BC1UnormSrgb,      RenderBackendTextureFormatType::UnormSrgb,    8,     3,        { false, false },    { 64, 0,  0,  0  } },
        { "BC2Unorm",          RenderBackendTextureFormat::BC2Unorm,          RenderBackendTextureFormatType::Unorm,        16,    4,        { false, false },    { 128, 0,  0, 0  } },
        { "BC2UnormSrgb",      RenderBackendTextureFormat::BC2UnormSrgb,      RenderBackendTextureFormatType::UnormSrgb,    16,    4,        { false, false },    { 128, 0,  0, 0  } },
        { "BC3Unorm",          RenderBackendTextureFormat::BC3Unorm,          RenderBackendTextureFormatType::Unorm,        16,    4,        { false, false },    { 128, 0,  0, 0  } },
        { "BC3UnormSrgb",      RenderBackendTextureFormat::BC3UnormSrgb,      RenderBackendTextureFormatType::UnormSrgb,    16,    4,        { false, false },    { 128, 0,  0, 0  } },
        { "BC4Unorm",          RenderBackendTextureFormat::BC4Unorm,          RenderBackendTextureFormatType::Unorm,        8,     1,        { false, false },    { 64, 0,  0,  0  } },
        { "BC4Snorm",          RenderBackendTextureFormat::BC4Snorm,          RenderBackendTextureFormatType::Snorm,        8,     1,        { false, false },    { 64, 0,  0,  0  } },
        { "BC5Unorm",          RenderBackendTextureFormat::BC5Unorm,          RenderBackendTextureFormatType::Unorm,        16,    2,        { false, false },    { 128, 0,  0, 0  } },
        { "BC5Snorm",          RenderBackendTextureFormat::BC5Snorm,          RenderBackendTextureFormatType::Snorm,        16,    2,        { false, false },    { 128, 0,  0, 0  } },

        { "BC6HUF",            RenderBackendTextureFormat::BC6HUF,            RenderBackendTextureFormatType::Float,        16,    3,        { false, false },    { 128, 0,  0, 0  } },
        { "BC6HSF",            RenderBackendTextureFormat::BC6HSF,            RenderBackendTextureFormatType::Float,        16,    3,        { false, false },    { 128, 0,  0, 0  } },
        { "BC7Unorm",          RenderBackendTextureFormat::BC7Unorm,          RenderBackendTextureFormatType::Unorm,        16,    4,        { false, false },    { 128, 0,  0, 0  } },
        { "BC7UnormSrgb",      RenderBackendTextureFormat::BC7UnormSrgb,      RenderBackendTextureFormatType::UnormSrgb,    16,    4,        { false, false },    { 128, 0,  0, 0  } },
    };

    static_assert(((uint32)(sizeof(GRenderBackendTextureFormatTable) / sizeof(*(GRenderBackendTextureFormatTable)))) == (uint32)RenderBackendTextureFormat::Count);

    const RenderBackendTextureFormatDesc& RenderBackendGetTextureFormatDesc(RenderBackendTextureFormat format)
    {
        return GRenderBackendTextureFormatTable[(uint32)format];
    }
}