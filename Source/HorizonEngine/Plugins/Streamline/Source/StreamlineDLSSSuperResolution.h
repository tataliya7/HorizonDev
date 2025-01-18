#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

namespace Horizon
{
    enum class DLSSSuperResolutionQualityMode
    {
        Off = 0,
        Auto = 1,
        Quality = 2,
        Balanced = 3,
        Performance = 4,
        UltraPerformance = 5,
        UltraQuality = 6,
    };

    enum class DLSSSuperResolutionAPI
    {
        Unknown,
        D3D12,
        Vulkan,
    };

    TemporalSuperSamplingInterface* StreamlineDLSSSuperResolutionCreate(RenderBackend* renderBackend);

    void StreamlineDLSSSuperResolutionDestroy(TemporalSuperSamplingInterface* temporalSuperSamplingInterface);
}