#pragma once

namespace HE
{
    enum class ColorSpace : uint8
    {
        sRGB = 0,
        Rec2020 = 1,
        ACES2065_1 = 2,
        ACEScct = 3,
        ACEScg = 4,
    };

    static ColorSpace WorkingColorSpace = ColorSpace::sRGB;
}