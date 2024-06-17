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
        Extent2D velocityTileCount = Extent2D();

        RenderGraphTextureDesc velocityAndDepthTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle velocityAndDepthTexture = renderGraph.CreateTexture(velocityAndDepthTextureDesc, "MotionBlurVelocityAndDepthTexture");

        RenderGraphTextureDesc velocityTileTextureDesc = RenderGraphTextureDesc::Create2D(
            velocityTileCount.width,
            velocityTileCount.height,
            RenderBackendTextureFormat::R16G16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle velocityTileTexture = renderGraph.CreateTexture(velocityTileTextureDesc, "MotionBlurVelocityTileTexture");
        RenderGraphTextureHandle dilatedVelocityTileTexture = renderGraph.CreateTexture(velocityTileTextureDesc, "MotionBlurDilatedVelocityTileTexture");

        return RenderGraphTextureHandle::Null;
    }
}