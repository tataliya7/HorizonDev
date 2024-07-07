#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::RenderScreenSpaceShadows(
        RenderGraph& renderGraph,
        const SceneView& view,
        const LightRenderObject& light,
        RenderGraphTextureHandle& screenSpaceShadowMaskTexture)
    {
        // uint32 numDynamicShadowCascades = light.GetNumDynamicShadowCascades();
        // // uint32 shadowMapSize = light.GetShadowMapSize();
        // uint32 shadowMapSize = 4096;
        //
        // RenderGraphTextureDesc shadowMapDesc = RenderGraphTextureDesc::Create2DArray(
        //     shadowMapSize,
        //     shadowMapSize,
        //     RenderBackendTextureFormat::D32Float,
        //     RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::DepthStencil,
        //     numDynamicShadowCascades,
        //     RenderBackendTextureClearValue::CreateDepthValue(FarClipPlaneDepthValue));
        //
        // auto shadowMap = renderGraph.CreateTexture(shadowMapDesc, "CascadedShadowMap");
        //
        // static bool renderSM = true;
        // if (renderSM)
        // {
        //     //renderSM = false;
        //     for (uint32 cascadeIndex = 0; cascadeIndex < numDynamicShadowCascades; cascadeIndex++)
        //     {
        //         renderGraph.AddPass("CascadedShadowMap", RenderGraphPassFlags::Graphics,
        //             [&](RenderGraphBuilder& builder)
        //             {
        //                 shadowMap = builder.WriteTexture(shadowMap, RenderBackendResourceState::DepthStencil);
        //
        //                 builder.BindDepthTarget(shadowMap, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve, 0, cascadeIndex);
        //
        //                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //                 {
        //                     RenderBackendViewport viewport(0.0f, 0.0f, (float)shadowMapSize, (float)shadowMapSize);
        //                     commandList.SetViewports(&viewport, 1);
        //
        //                     RenderBackendScissor scissor(0, 0, shadowMapSize, shadowMapSize);
        //                     commandList.SetScissors(&scissor, 1);
        //
        //                     RenderBackendGraphicsPipelineState graphicsPipelineState = {};
        //                     graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
        //                     // TODO: vkCmdSetDepthBias
        //                     graphicsPipelineState.rasterizationState.depthBiasConstantFactor = light.shadowMapDepthBiasConstantFactor;
        //                     graphicsPipelineState.rasterizationState.depthBiasSlopeFactor = light.shadowMapDepthBiasSlopeFactor;
        //                     graphicsPipelineState.depthStencilState.depthTestEnable = true;
        //                     graphicsPipelineState.depthStencilState.depthWriteEnable = true;
        //                     graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;
        //
        //                     RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::CascadedShadowMap);
        //
        //                     for (const auto& drawCallInfo : renderEngine->drawList)
        //                     {
        //                         RenderBackendShaderConstants shaderConstants = {};
        //                         shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //                         shaderConstants.BindBuffer(1, renderEngine->geometryBuffer, drawCallInfo.geometryIndex * sizeof(GeometryShaderParameters));
        //                         shaderConstants.BindBuffer(2, renderEngine->materialBuffer, 0);
        //                         shaderConstants.BindBuffer(3, renderEngine->cascadedShadowMapBuffer, 0);
        //                         shaderConstants.PushConstants(0, 1.0f * cascadeIndex);
        //
        //                         commandList.DrawIndexed(
        //                             graphicsShader,
        //                             graphicsPipelineState,
        //                             shaderConstants,
        //                             drawCallInfo.indexBuffer,
        //                             drawCallInfo.numIndices,
        //                             1,
        //                             drawCallInfo.firstIndex,
        //                             0,
        //                             0,
        //                             RenderBackendPrimitiveTopology::TriangleList);
        //                     }
        //                 };
        //             });
        //     }
        // }
        //
        // renderGraph.AddPass("ScreenSpaceShadows", RenderGraphPassFlags::Compute,
        //     [&](RenderGraphBuilder& builder)
        //     {
        //         auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        //
        //         auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
        //         shadowMap = builder.ReadTexture(shadowMap, RenderBackendResourceState::ShaderResource);
        //
        //         screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);
        //
        //         return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //         {
        //             RenderBackendViewport viewport(0.0f, 0.0f, (float)renderResolution.width, (float)renderResolution.height);
        //             commandList.SetViewports(&viewport, 1);
        //
        //             RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
        //             commandList.SetScissors(&scissor, 1);
        //
        //             uint32 threadGroupCountX = ComputeWorkGroupCount(renderResolution.width, 8);
        //             uint32 threadGroupCountY = ComputeWorkGroupCount(renderResolution.height, 8);
        //             uint32 threadGroupCountZ = 1;
        //
        //             RenderBackendShaderConstants shaderConstants = {};
        //             shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //             shaderConstants.BindBuffer(4, renderEngine->cascadedShadowMapBuffer, 0);
        //             shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture)));
        //             shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(shadowMap)));
        //             shaderConstants.BindTextureUAV(5, registry.GetTextureUAVBindlessResourceDescriptorIndexscreenSpaceShadowMaskTexture), 0));
        //
        //             RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::ScreenSpaceShadowsDirectionalLight);
        //             commandList.Dispatch(
        //                 computeShader,
        //                 shaderConstants,
        //                 threadGroupCountX,
        //                 threadGroupCountY,
        //                 threadGroupCountZ);
        //         };
        //     });
    }
}