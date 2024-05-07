#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::AddMotionBlurPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        return RenderGraphTextureHandle::Null;
    }
}