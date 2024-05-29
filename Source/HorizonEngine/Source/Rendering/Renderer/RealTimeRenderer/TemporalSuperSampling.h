#pragma once

#include "RealTimeRendererCommon.h"

namespace Horizon
{
    struct TemporalSuperSamplingDispatchParameters
    {

    };

    class TemporalSuperSamplingInterface
    {
    public:
        virtual RenderGraphTextureHandle Dispatch(RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchParameters& dispatchParameters) const = 0;
    };
}