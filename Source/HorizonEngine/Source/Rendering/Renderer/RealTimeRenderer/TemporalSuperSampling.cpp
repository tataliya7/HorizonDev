#include "RealTimeRenderer.h"
#include "TemporalSuperSampling.h"

namespace Horizon
{
    bool RealTimeRenderer::IsSuperResolutionEnabled() const
    {
        return features.enableSuperResolution;
    }
}