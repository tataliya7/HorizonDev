#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    struct RenderStatistics
    {
        uint64 meshCount = 0;
        uint64 meshInstanceCount = 0;
        uint64 transformCount = 0;
        uint64 triangleCount = 0;
        uint64 vertexCount = 0;
        uint64 instancedTriangleCount = 0;
        uint64 instancedVertexCount = 0;
        uint64 indexMemoryInBytes = 0;
        uint64 vertexMemoryInBytes = 0;
        uint64 geometryMemoryInBytes = 0;
        uint64 animationMemoryInBytes = 0;
        uint64 totalLightCount = 0;
        uint64 lightsMemoryInBytes = 0;
        uint64 environmentMapMemoryInBytes = 0;
    };
}