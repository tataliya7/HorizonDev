#pragma once

#include "RasterizationRendererCommon.h"

namespace Horizon
{
    struct GPUSceneLocalLightShaderParameters
    {
        Vector4f data0;
        Vector4f data1;
        Vector4f data2;
        Vector4f data3;
        Vector4f data4;
    };

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