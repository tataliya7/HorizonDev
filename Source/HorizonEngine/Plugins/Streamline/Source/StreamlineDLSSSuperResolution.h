#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

namespace Streamline
{
    enum class DLSSQualityMode
    {
        Off = 0,
        Auto = 1,
        Quality = 2,
        Balanced = 3,
        Performance = 4,
        UltraPerformance = 5,
    };

    struct DLSSSettings
    {
        DLSSQualityMode qualityMode = DLSSQualityMode::Auto;
    };

    class StreamlineDLSS : public TemporalSuperSamplingInterface
    {
    public:
        AddPass() override;
    private:

    };
}