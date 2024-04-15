#include "RealTimeRenderer.h"

namespace HE
{
    void RealTimeRenderer::RenderVisibilityBuffer(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        if (true)
        {
            renderGraph.AddPass("VisibilityBuffer", RenderGraphPassFlags::Graphics,
                [&](RenderGraphBuilder& builder)
                {
                    auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                    auto vbuffer0 = sceneTextures.vbuffer0 = builder.WriteTexture(sceneTextures.vbuffer0, RenderBackendResourceState::RenderTarget);
                    auto vbuffer1 = sceneTextures.vbuffer1 = builder.WriteTexture(sceneTextures.vbuffer1, RenderBackendResourceState::RenderTarget);
                    auto sceneDepthTexture = sceneTextures.sceneDepthTexture = builder.WriteTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);

                    builder.BindColorTarget(0, vbuffer0, RenderBackendRenderTargetLoadOp::Clear, RenderBackendRenderTargetStoreOp::Store);
                    builder.BindColorTarget(1, vbuffer1, RenderBackendRenderTargetLoadOp::Clear, RenderBackendRenderTargetStoreOp::Store);
                    builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderTargetLoadOp::Clear, RenderBackendRenderTargetStoreOp::Store);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        RenderBackendViewport viewport(0.0f, 0.0f, (float)renderResolutionX, (float)renderResolutionY);
                        commandList.SetViewports(&viewport, 1);

                        RenderBackendScissor scissor(0, 0, renderResolutionX, renderResolutionY);
                        commandList.SetScissors(&scissor, 1);

                        RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::VBuffer);

                        RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
                        graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
                        graphicsPipelineState.depthStencilState.depthTestEnable = true;
                        graphicsPipelineState.depthStencilState.depthWriteEnable = true;
                        graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

                        for (const auto& drawCallInfo : renderEngine->drawList)
                        {
                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.debugName = "VisibilityBuffer";
                            shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                            shaderArguments.BindBuffer(1, renderEngine->geometryBuffer, drawCallInfo.geometryIndex * sizeof(GeometryShaderParameters));
                            shaderArguments.BindBuffer(2, renderEngine->materialBuffer, 0);
                            shaderArguments.BindBuffer(3, drawCallInfo.vertexBuffers[0], 0);
                            shaderArguments.BindBuffer(4, drawCallInfo.vertexBuffers[1], 0);
                            shaderArguments.BindBuffer(5, drawCallInfo.vertexBuffers[2], 0);
                            shaderArguments.BindBuffer(6, drawCallInfo.vertexBuffers[3], 0);
                            shaderArguments.PushConstants(0, (float)drawCallInfo.geometryIndex);

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
        else
        {
            renderGraph.AddPass(std::format("VisibilityBuffer"), RenderGraphPassFlags::Graphics,
                [&](RenderGraphBuilder& builder)
                {
                    auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                    auto vbuffer0 = sceneTextures.vbuffer0 = builder.WriteTexture(sceneTextures.vbuffer0, RenderBackendResourceState::RenderTarget);
                    auto vbuffer1 = sceneTextures.vbuffer1 = builder.WriteTexture(sceneTextures.vbuffer1, RenderBackendResourceState::RenderTarget);
                    auto sceneDepthTexture = sceneTextures.sceneDepthTexture = builder.WriteTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);

                    builder.BindColorTarget(0, vbuffer0, RenderBackendRenderTargetLoadOp::Clear, RenderBackendRenderTargetStoreOp::Store);
                    builder.BindColorTarget(1, vbuffer1, RenderBackendRenderTargetLoadOp::Clear, RenderBackendRenderTargetStoreOp::Store);
                    builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderTargetLoadOp::Clear, RenderBackendRenderTargetStoreOp::Store);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        RenderBackendViewport viewport(0.0f, 0.0f, (float)renderResolutionX, (float)renderResolutionY);
                        commandList.SetViewports(&viewport, 1);

                        RenderBackendScissor scissor(0, 0, renderResolutionX, renderResolutionY);
                        commandList.SetScissors(&scissor, 1);

                        RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::VBufferMeshlet);

                        RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
                        graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
                        graphicsPipelineState.depthStencilState.depthTestEnable = true;
                        graphicsPipelineState.depthStencilState.depthWriteEnable = true;
                        graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

                        for (const auto& drawCallInfo : renderEngine->drawList)
                        {
                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                            shaderArguments.BindBuffer(1, renderEngine->geometryBuffer, drawCallInfo.geometryIndex * sizeof(GeometryShaderParameters));
                            shaderArguments.BindBuffer(2, renderEngine->materialBuffer, 0);
                            shaderArguments.PushConstants(0, (float)drawCallInfo.geometryIndex);

                            uint32 meshletCount = 1;
                            commandList.DisptachMesh(
                                graphicsShader,
                                graphicsPipelineState,
                                shaderArguments,
                                meshletCount,
                                1,
                                1,
                                RenderBackendPrimitiveTopology::TriangleList);
                        }
                    };
                });
        }
    }

    void RealTimeRenderer::RenderGBuffer(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        renderGraph.AddPass(std::format("GBuffer (Compute, {}x{})", renderResolutionX, renderResolutionY), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                auto vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);
                auto vbuffer1 = builder.ReadTexture(sceneTextures.vbuffer1, RenderBackendResourceState::ShaderResource);

                auto gbuffer0 = sceneTextures.gbuffer0 = builder.WriteTexture(sceneTextures.gbuffer0, RenderBackendResourceState::UnorderedAccess);
                auto gbuffer1 = sceneTextures.gbuffer1 = builder.WriteTexture(sceneTextures.gbuffer1, RenderBackendResourceState::UnorderedAccess);
                auto gbuffer2 = sceneTextures.gbuffer2 = builder.WriteTexture(sceneTextures.gbuffer2, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.debugName = "GBuffer";
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindBuffer(1, renderEngine->geometryBuffer, 0);
                    shaderArguments.BindBuffer(2, renderEngine->materialBuffer, 0);
                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(vbuffer0)));
                    shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(vbuffer1)));
                    shaderArguments.BindTextureUAV(5, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(gbuffer0)));
                    shaderArguments.BindTextureUAV(6, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(gbuffer1)));
                    shaderArguments.BindTextureUAV(7, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(gbuffer2)));

                    uint32 dispatchX = Math::CeilDiv(renderResolutionX, 8);
                    uint32 dispatchY = Math::CeilDiv(renderResolutionY, 8);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::GBuffer);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        dispatchX,
                        dispatchY);
                };
            });
    }

    void RealTimeRenderer::RenderMotionVectors(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        renderGraph.AddPass("MotionVectors", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                auto vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);

                auto motionVectors = sceneTextures.motionVectorTexture = builder.WriteTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 dispatchX = Math::CeilDiv(renderResolutionX, 8);
                    uint32 dispatchY = Math::CeilDiv(renderResolutionY, 8);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindBuffer(1, renderEngine->geometryBuffer, 0);
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(vbuffer0)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(motionVectors), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::MotionVectors);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        dispatchX,
                        dispatchY);
                };
            });
    }

    void RealTimeRenderer::AddDirectLightingPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle screenSpaceShadowMaskTexture,
        RenderGraphTextureHandle localLightShadowMapAtlas)
    {
        renderGraph.AddPass(std::format("DirectLighting (Graphics, {}x{})", renderResolutionX, renderResolutionY), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                auto& skyAtmosphereData = renderGraph.blackboard.Get<RenderGraphSkyAtmosphereData>();

                auto vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);
                auto vbuffer1 = builder.ReadTexture(sceneTextures.vbuffer1, RenderBackendResourceState::ShaderResource);
                auto gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
                auto gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
                auto gbuffer2 = builder.ReadTexture(sceneTextures.gbuffer2, RenderBackendResourceState::ShaderResource);
                // TODO: which state should be?
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                //auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);
                screenSpaceShadowMaskTexture = builder.ReadTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::ShaderResource);
                localLightShadowMapAtlas = builder.ReadTexture(localLightShadowMapAtlas, RenderBackendResourceState::ShaderResource);
                auto skyAtmosphereTransmittanceLUT = builder.ReadTexture(skyAtmosphereData.transmittanceLut, RenderBackendResourceState::ShaderResource);

                auto sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, sceneColorTexture, RenderBackendRenderTargetLoadOp::Load, RenderBackendRenderTargetStoreOp::Store);
                builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderTargetLoadOp::Load, RenderBackendRenderTargetStoreOp::Store);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.debugName = "DirectLighting";
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneDepthTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(vbuffer0)));
                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(vbuffer1)));
                    shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer0)));
                    shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer1)));
                    shaderArguments.BindTextureSRV(10, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer2)));
                    shaderArguments.BindTextureSRV(8, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(screenSpaceShadowMaskTexture)));
                    shaderArguments.BindTextureSRV(9, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(localLightShadowMapAtlas)));
                    shaderArguments.BindTextureSRV(15, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(skyAtmosphereTransmittanceLUT)));
                    shaderArguments.BindBuffer(11, renderEngine->geometryBuffer, 0); // TODO: fix crash when geometryBuffer is null
                    shaderArguments.BindBuffer(12, renderEngine->materialBuffer, 0);
                    shaderArguments.BindBuffer(13, renderEngine->lightDataBuffer, 0);
                    shaderArguments.BindBuffer(14, renderEngine->cubeShadowMapBuffer, 0);
                    shaderArguments.PushConstants(0, (float)renderEngine->numLights);
                    //shaderArguments.PushConstants(0, (float)renderEngine->numLights);

                    // struct PassParameters
                    // {
                    //     uint32 numLights;
                    // };
                    // PassParameters parameters;
                    // parameters.numLights = renderEngine->numLights;
                    // shaderArguments.PushConstantsTest(&parameters, sizeof(PassParameters));

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcAlphaBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstAlphaBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].alphaBlendOp = RenderBackendBlendOp::Add;
                    graphicsPipelineState.colorBlendState.targetBlends[0].colorWriteMask = RenderBackendColorComponentFlags::RGBA;

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::DirectLighting);
                    commandList.Draw(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderArguments,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }

    void RealTimeRenderer::AddIndirectLightingDiffusePass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        renderGraph.AddPass(std::format("IndirectLightingDiffuse (Graphics, {}x{})", renderResolutionX, renderResolutionY), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                auto gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
                auto gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
                auto gbuffer2 = builder.ReadTexture(sceneTextures.gbuffer2, RenderBackendResourceState::ShaderResource);
                auto ambientOcclusionTexture = builder.ReadTexture(sceneTextures.ambientOcclusionTexture, RenderBackendResourceState::ShaderResource);
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);

                auto sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                // TODO: move clear render target to a separate pass
                builder.BindColorTarget(0, sceneColorTexture, RenderBackendRenderTargetLoadOp::Clear, RenderBackendRenderTargetStoreOp::Store);
                builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderTargetLoadOp::Load, RenderBackendRenderTargetStoreOp::Store);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer0)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer1)));
                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer2)));
                    shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(ambientOcclusionTexture)));
                    shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(renderEngine->irradianceEnvironmentMap));
                    shaderArguments.BindBuffer(6, renderEngine->irradianceEnvironmentMapSH, 0);

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::IndirectLightingDiffuse);
                    commandList.Draw(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderArguments,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }

    void RealTimeRenderer::AddIndirectLightingSpecularPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        renderGraph.AddPass(std::format("IndirectLightingSpecular (Graphics, {}x{})", renderResolutionX, renderResolutionY), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                auto gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
                auto gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
                auto gbuffer2 = builder.ReadTexture(sceneTextures.gbuffer2, RenderBackendResourceState::ShaderResource);
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);
                auto ambientOcclusionTexture = builder.ReadTexture(sceneTextures.ambientOcclusionTexture, RenderBackendResourceState::ShaderResource);

                auto sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, sceneColorTexture, RenderBackendRenderTargetLoadOp::Load, RenderBackendRenderTargetStoreOp::Store);
                builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderTargetLoadOp::Load, RenderBackendRenderTargetStoreOp::Store);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0] = additiveRGBAColorBlendAttachmentState;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer0)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer1)));
                    shaderArguments.BindTextureSRV(7, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer2)));
                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneDepthTexture)));
                    shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(ambientOcclusionTexture)));
                    shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(renderEngine->GetDefaultResources().GetPreIntegratedBRDFLUT()));
                    shaderArguments.BindTextureSRV(6, RenderBackendTextureSRVDesc::Create(renderEngine->filteredEnvironmentMap));

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::IndirectLightingSpecular);
                    commandList.Draw(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderArguments,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }
}