#pragma once

#include "PathTracingRendererCommon.h"

namespace Horizon
{
    enum class PathTracingMode: uint8
    {
        RealTime     = 0,
        Reference    = 1
    };

    struct PathTracingRendererSettings
    {
        uint32 samplesPerPixel;
        uint32 maxBounces;
    };
}