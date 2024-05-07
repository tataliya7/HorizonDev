#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::RenderRayTracingShadows(
        RenderGraph& renderGraph,
        const SceneView& view,
        const LightComponent& light,
        RenderGraphTextureHandle& screenSpaceShadowMaskTexture,
        RenderGraphTextureHandle& rayDistanceTexture)
    {
        renderGraph.AddPass("RayTracingShadowsPass", RenderGraphPassFlags::RayTracing,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);

                screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 width = renderResolution.width;
                    uint32 height = renderResolution.height;

                    RenderBackendRayTracingAccelerationStructureHandle rayTracingScene = view.GetRayTracingScene();

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindAS(1, rayTracingScene);
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(screenSpaceShadowMaskTexture), 0));
                    shaderArguments.BindTextureUAV(4, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(rayDistanceTexture), 0));
                    shaderArguments.PushConstants(0, light.GetDirection().x);
                    shaderArguments.PushConstants(1, light.GetDirection().y);
                    shaderArguments.PushConstants(2, light.GetDirection().z);

                    commandList.TraceRays(
                        rayTracingShadowsPipelineState,
                        rayTracingShadowsSBT,
                        shaderArguments,
                        width,
                        height,
                        1);
                };
            });
    }
}