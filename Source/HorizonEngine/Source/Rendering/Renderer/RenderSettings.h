#pragma once

#include "RendererCommon.h"
#include "RasterizationRenderer/RasterizationRendererSettings.h"
#include "PathTracingRenderer/PathTracingRendererSettings.h"

namespace Horizon
{
    enum class RenderMode
    {
        RasterRendering,
        HybridRendering,
        RealTimePathTracing,
        ReferencePathTracing,
    };

    /**
     * TBD.
     */
    struct RenderSettings
    {
        RenderMode renderMode;
        RasterizationRendererSettings rasterRenderingSettings;
        RasterizationRendererSettings hybridRenderingSettings;
        PathTracingRendererSettings realTimePathTracingSettings;
        PathTracingRendererSettings referencePathTracingSettings;
    };
}