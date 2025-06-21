#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    static constexpr float NearClippingPlaneDepthValue = 1.0f;
    static constexpr float FarClippingPlaneDepthValue = 0.0f;

    enum class RenderMode
    {
        RasterRendering,
        HybridRendering,
        RealTimePathTracing,
        ReferencePathTracing,
    };
}