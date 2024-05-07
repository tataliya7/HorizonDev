#pragma once

#include "RealTimeRendererCommon.h"

namespace Horizon
{
    class TemporalSuperSamplingInterface
    {
    public:
        virtual FOutputs AddPass(FRDGBuilder& GraphBuilder, onst FSceneView& View, const FInputs& Inputs) const = 0;
    };
}