#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"

namespace HE
{
    RenderGraphTextureHandle RealTimeRenderer::AddDLSSPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        RenderGraphTextureHandle sceneDepthTexture,
        RenderGraphTextureHandle motionVectorTexture)
    {
        RenderGraphTextureDesc dlssOutputTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolutionX,
            targetResolutionY,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle dlssOutputTexture = renderGraph.CreateTexture(dlssOutputTextureDesc, "DLSSOutputTexture");

        uint32 frameIndex = sceneViewShaderParameters.frameIndex;
        bool reset = (frameIndex == 0) ? true : false;

        renderGraph.AddPass("EvaluateDLSSPass", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);

                dlssOutputTexture = builder.WriteTexture(dlssOutputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.EvaluateDLSS(
                        registry.GetRenderBackendTexture(dlssOutputTexture),
                        registry.GetRenderBackendTexture(sceneColorTexture),
                        registry.GetRenderBackendTexture(sceneDepthTexture),
                        registry.GetRenderBackendTexture(motionVectorTexture),
                        targetResolutionX,
                        targetResolutionY,
                        frameIndex);
                };
            });

        return dlssOutputTexture;
    }
}