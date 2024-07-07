#include "RealTimeRenderer.h"

namespace Horizon
{
    bool ShouldRenderRayTracingShadowsForLight(const LightRenderObject& light)
    {
        if (light.CastRayTracingShadows())
        {
            return true;
        }
        return false;
    }

    void RealTimeRenderer::RenderRayTracingShadows(
        RenderGraph& renderGraph,
        const SceneView& view,
        const LightRenderObject& light,
        RenderGraphTextureHandle& screenSpaceShadowMaskTexture,
        RenderGraphTextureHandle& rayDistanceTexture)
    {
        // renderGraph.AddPass(
        //     "RayTracingShadowsPass",
        //     RenderGraphPassFlags::RayTracing,
        //     [&](RenderGraphBuilder& builder)
        //     {
        //         auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        //
        //         auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
        //
        //         screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);
        //
        //         return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //         {
        //             uint32 width = renderResolution.width;
        //             uint32 height = renderResolution.height;
        //
        //             RenderBackendRayTracingAccelerationStructureHandle rayTracingScene = view.GetRayTracingScene();
        //
        //             RenderBackendShaderConstants shaderConstants = {};
        //             shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //             shaderConstants.BindAS(1, rayTracingScene);
        //             shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture)));
        //             shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndexscreenSpaceShadowMaskTexture), 0));
        //             shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndexrayDistanceTexture), 0));
        //
        //             shaderConstants.PushConstants(0, light.GetDirection().x);
        //             shaderConstants.PushConstants(1, light.GetDirection().y);
        //             shaderConstants.PushConstants(2, light.GetDirection().z);
        //
        //             commandList.DispatchRays(
        //                 rayTracingShadowsPipelineState,
        //                 rayTracingShadowsSBT,
        //                 shaderConstants,
        //                 width,
        //                 height,
        //                 1);
        //         };
        //     });
    }
}