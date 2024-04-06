#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace HE
{
    RenderGraphTextureHandle RealTimeRenderer::AddMotionBlurPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        return RenderGraphTextureHandle::Null;
    }
}