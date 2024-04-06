#pragma once

#include <stdint.h>

namespace HE
{
    using int8 = int8_t;
    using int16 = int16_t;
    using int32 = int32_t;
    using int64 = int64_t;

    using uint8 = uint8_t;
    using uint16 = uint16_t;
    using uint32 = uint32_t;
    using uint64 = uint64_t;

    using char8 = char8_t;
    using char16 = char16_t;
    using char32 = char32_t;
    
    struct Extent2D
    {
        uint32 width;
        uint32 height;
    };

    struct Extent3D
    {
        uint32 width;
        uint32 height;
        uint32 depth;
    };

    struct Offset2D
    {
        int32 x;
        int32 y;
    };

    struct Offset3D
    {
        int32 x;
        int32 y;
        int32 z;
    };
}
