#pragma once

#include "RasterizationRendererCommon.h"

namespace Horizon
{
    struct LightGridInfo
    {
        uint32 localLightCount;
        uint32 cellCount;
        uint32 lightGridSizeX;
        uint32 lightGridSizeY;
        uint32 lightGridSizeZ;
        uint32 maxLightCountPerCell;
    };
}