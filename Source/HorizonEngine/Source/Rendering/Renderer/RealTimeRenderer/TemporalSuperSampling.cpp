#include "RealTimeRenderer.h"
#include "TemporalSuperSampling.h"

namespace Horizon
{
    bool RealTimeRenderer::IsSuperResolutionEnabled() const
    {
        return features.enableSuperResolution;
    }

    RenderGraphTextureHandle DispatchCustomTemporalSuperSampling(RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchParameters& dispatchParameters)
    {
        return RenderGraphTextureHandle::Null;
    }
}