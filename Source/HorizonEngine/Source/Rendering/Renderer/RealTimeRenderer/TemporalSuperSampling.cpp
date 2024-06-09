#include "RealTimeRenderer.h"
#include "TemporalSuperSampling.h"

namespace Horizon
{
    bool RealTimeRenderer::IsSuperResolutionEnabled() const
    {
        return features.enableSuperResolution;
    }

    RenderGraphTextureHandle DispatchCustomTemporalSuperSampling(TemporalSuperSamplingInterface* interface, RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchDescription& dispatchDescription)
    {
        RenderGraphTextureHandle outputTexture = interface->Dispatch(renderGraph, view, dispatchDescription);
        return outputTexture;
    }
}