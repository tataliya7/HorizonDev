#pragma once

#include "RealTimeRendererCommon.h"

namespace Horizon
{
    struct TemporalSuperSamplingDispatchParameters
    {
        RenderGraphTextureHandle colorTexture;
        RenderGraphTextureHandle depthTexture;
        RenderGraphTextureHandle motionVectorTexture;
    };

    class TemporalSuperSamplingInterface
    {
    public:
        virtual RenderGraphTextureHandle Dispatch(RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchParameters& dispatchParameters) const = 0;
    };

    RenderGraphTextureHandle DispatchCustomTemporalSuperSampling(RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchParameters& dispatchParameters);
}