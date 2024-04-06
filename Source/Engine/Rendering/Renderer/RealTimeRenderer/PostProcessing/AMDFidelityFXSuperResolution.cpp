#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"

namespace HE
{
    RenderGraphTextureHandle RealTimeRenderer::AddFSR2Pass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        RenderGraphTextureHandle sceneDepthTexture,
        RenderGraphTextureHandle motionVectorTexture)
    {
        bool reset = sceneViewShaderParameters.frameIndex == 0 ? true : false;

        RenderGraphTextureHandle fsr2OutputTexture = renderGraph.CreateTexture(
            RenderGraphTextureDesc::Create2D(
                targetResolutionX,
                targetResolutionY,
                RenderBackendTextureFormat::RGBA16Float,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess),
            "FSR2Texture");

        renderGraph.AddPass("FSR2Dispatch", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                auto sceneColorTexture = builder.ReadTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::ShaderResource);
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                auto motionVectorTexture = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);

                fsr2OutputTexture = builder.WriteTexture(fsr2OutputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.FSR2Dispatch(
                        registry.GetRenderBackendTexture(fsr2OutputTexture),
                        registry.GetRenderBackendTexture(sceneColorTexture),
                        registry.GetRenderBackendTexture(sceneDepthTexture),
                        registry.GetRenderBackendTexture(motionVectorTexture),
                        sceneViewShaderParameters.renderResolutionX,
                        sceneViewShaderParameters.renderResolutionY,
                        sceneViewShaderParameters.targetResolutionX,
                        sceneViewShaderParameters.targetResolutionY,
                        sceneViewShaderParameters.cameraJitterOffset.x,
                        sceneViewShaderParameters.cameraJitterOffset.y,
                        (float)sceneViewShaderParameters.renderResolutionX,
                        (float)sceneViewShaderParameters.renderResolutionY,
                        reset,
                        sceneViewShaderParameters.deltaTime,
                        settings.fsr2Settings.useRCAS,
                        settings.fsr2Settings.sharpeness,
                        sceneViewShaderParameters.cameraFarPlane,
                        sceneViewShaderParameters.cameraNearPlane,
                        2.0f * sceneViewShaderParameters.cameraHalfFovRad);
                };
            });

        return fsr2OutputTexture;
    }
}