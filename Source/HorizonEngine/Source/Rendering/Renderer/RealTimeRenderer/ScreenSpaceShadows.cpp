#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::RenderScreenSpaceShadows(
        RenderGraph& renderGraph,
        const SceneView& view,
        const LightComponent& light,
        RenderGraphTextureHandle& screenSpaceShadowMaskTexture)
    {
        uint32 numDynamicShadowCascades = light.GetNumDynamicShadowCascades();
        // uint32 shadowMapSize = light.GetShadowMapSize();
        uint32 shadowMapSize = 4096;

        RenderGraphTextureDesc shadowMapDesc = RenderGraphTextureDesc::Create2DArray(
            shadowMapSize,
            shadowMapSize,
            RenderBackendTextureFormat::D32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::DepthStencil,
            numDynamicShadowCascades,
            RenderBackendTextureClearValue::CreateDepthValue(FarClipPlaneDepthValue));

        auto shadowMap = renderGraph.CreateTexture(shadowMapDesc, "CascadedShadowMap");

        static bool renderSM = true;
        if (renderSM)
        {
            //renderSM = false;
            for (uint32 cascadeIndex = 0; cascadeIndex < numDynamicShadowCascades; cascadeIndex++)
            {
                renderGraph.AddPass("CascadedShadowMap", RenderGraphPassFlags::Graphics,
                    [&](RenderGraphBuilder& builder)
                    {
                        shadowMap = builder.WriteTexture(shadowMap, RenderBackendResourceState::DepthStencil);

                        builder.BindDepthTarget(shadowMap, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve, 0, cascadeIndex);

                        return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                        {
                            RenderBackendViewport viewport(0.0f, 0.0f, (float)shadowMapSize, (float)shadowMapSize);
                            commandList.SetViewports(&viewport, 1);

                            RenderBackendScissor scissor(0, 0, shadowMapSize, shadowMapSize);
                            commandList.SetScissors(&scissor, 1);

                            RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                            graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
                            // TODO: vkCmdSetDepthBias
                            graphicsPipelineState.rasterizationState.depthBiasConstantFactor = light.shadowMapDepthBiasConstantFactor;
                            graphicsPipelineState.rasterizationState.depthBiasSlopeFactor = light.shadowMapDepthBiasSlopeFactor;
                            graphicsPipelineState.depthStencilState.depthTestEnable = true;
                            graphicsPipelineState.depthStencilState.depthWriteEnable = true;
                            graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

                            RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle(ShaderID::CascadedShadowMap);

                            for (const auto& drawCallInfo : renderEngine->drawList)
                            {
                                RenderBackendShaderArguments shaderArguments = {};
                                shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                                shaderArguments.BindBuffer(1, renderEngine->geometryBuffer, drawCallInfo.geometryIndex * sizeof(GeometryShaderParameters));
                                shaderArguments.BindBuffer(2, renderEngine->materialBuffer, 0);
                                shaderArguments.BindBuffer(3, renderEngine->cascadedShadowMapBuffer, 0);
                                shaderArguments.PushConstants(0, 1.0f * cascadeIndex);

                                commandList.DrawIndexed(
                                    graphicsShader,
                                    graphicsPipelineState,
                                    shaderArguments,
                                    drawCallInfo.indexBuffer,
                                    drawCallInfo.numIndices,
                                    1,
                                    drawCallInfo.firstIndex,
                                    0,
                                    0,
                                    RenderBackendPrimitiveTopology::TriangleList);
                            }
                        };
                    });
            }
        }

        renderGraph.AddPass("ScreenSpaceShadows", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                shadowMap = builder.ReadTexture(shadowMap, RenderBackendResourceState::ShaderResource);

                screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, (float)renderResolutionX, (float)renderResolutionY);
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, renderResolutionX, renderResolutionY);
                    commandList.SetScissors(&scissor, 1);

                    uint32 groupCountX = ComputeWorkGroupCount(renderResolutionX, 8);
                    uint32 groupCountY = ComputeWorkGroupCount(renderResolutionY, 8);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindBuffer(4, renderEngine->cascadedShadowMapBuffer, 0);
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(shadowMap)));
                    shaderArguments.BindTextureUAV(5, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(screenSpaceShadowMaskTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::ScreenSpaceShadowsDirectionalLight);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
                };
            });
    }
}