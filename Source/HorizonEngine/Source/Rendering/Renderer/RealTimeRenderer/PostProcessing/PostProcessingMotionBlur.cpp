#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    bool RealTimeRenderer::IsMotionBlurEnabled() const
    {
        return features.enableMotionBlur;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddMotionBlurPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        return RenderGraphTextureHandle::Null;
    }
}