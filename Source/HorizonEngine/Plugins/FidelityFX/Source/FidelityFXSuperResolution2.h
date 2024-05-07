#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

namespace FidelityFX
{
    enum class FSR2QualityMode
    {
        Custom = 0,
        Quality = 1,
        Balanced = 2,
        Performance = 3,
        UltraPerformance = 4,
    };

    struct FSR2Settings
    {
        bool useRCAS = false;
        float sharpeness = 1.0f;
        FSR2QualityMode qualityMode = FSR2QualityMode::Custom;
        float customUpscaleRatio = 1.0f;
    };

    class FidelityFXSuperResolution2 : public TemporalSuperSamplingInterface
    {
    public:
        AddPass() override;
    private:

    };
}