#pragma once

#include "RealTimeRendererCommon.h"

namespace Horizon
{
    class TemporalSuperSamplingInterface
    {
    public:
        virtual RenderGraphTextureHandle Dispatch(RenderGraph& renderGraph, onst FSceneView& View, const FInputs& Inputs) const = 0;
    };
}