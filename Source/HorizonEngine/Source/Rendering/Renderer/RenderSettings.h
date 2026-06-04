#pragma once

#include "RendererCommon.h"
#include "PostProcessing/PostProcessingSettings.h"
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

    struct RenderSettings
    {
        RenderMode renderMode;
        PostProcessingSettings postProcessingSettings;
        RasterizationRendererSettings rasterRenderingSettings;
        RasterizationRendererSettings hybridRenderingSettings;
        PathTracingRendererSettings realTimePathTracingSettings;
        PathTracingRendererSettings referencePathTracingSettings;
    };
}