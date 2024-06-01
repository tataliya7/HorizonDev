#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::RenderVisibilityBuffer(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
#if 0
        if (true)
        {
            renderGraph.AddPass("VisibilityBuffer", RenderGraphPassFlags::Graphics,
                [&](RenderGraphBuilder& builder)
                {
                    auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                    auto vbuffer0 = sceneTextures.vbuffer0 = builder.WriteTexture(sceneTextures.vbuffer0, RenderBackendResourceState::RenderTarget);
                    auto vbuffer1 = sceneTextures.vbuffer1 = builder.WriteTexture(sceneTextures.vbuffer1, RenderBackendResourceState::RenderTarget);
                    auto sceneDepthTexture = sceneTextures.sceneDepthTexture = builder.WriteTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);

                    builder.BindColorTarget(0, vbuffer0, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);
                    builder.BindColorTarget(1, vbuffer1, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);
                    builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        RenderBackendViewport viewport(0.0f, 0.0f, (float)renderResolution.width, (float)renderResolution.height);
                        commandList.SetViewports(&viewport, 1);

                        RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                        commandList.SetScissors(&scissor, 1);

                        RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::VisibilityBufferVS);
                        RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::VisibilityBufferPS);

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
                            shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                            shaderArguments.BindBuffer(1, renderEngine->geometryBuffer, drawCallInfo.geometryIndex * sizeof(GeometryShaderParameters));
                            shaderArguments.BindBuffer(2, renderEngine->materialBuffer, 0);
                            shaderArguments.BindBuffer(3, drawCallInfo.vertexBuffers[0], 0);
                            shaderArguments.BindBuffer(4, drawCallInfo.vertexBuffers[1], 0);
                            shaderArguments.BindBuffer(5, drawCallInfo.vertexBuffers[2], 0);
                            shaderArguments.BindBuffer(6, drawCallInfo.vertexBuffers[3], 0);
                            shaderArguments.PushConstants(0, (float)drawCallInfo.geometryIndex);

                            commandList.DrawIndexed(
                                vertexShader,
                                pixelShader,
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
            renderGraph.AddPass(std::format("VisibilityBuffer"), RenderGraphPassFlags::MeshShading,
                [&](RenderGraphBuilder& builder)
                {
                    auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                    auto vbuffer0 = sceneTextures.vbuffer0 = builder.WriteTexture(sceneTextures.vbuffer0, RenderBackendResourceState::RenderTarget);
                    auto vbuffer1 = sceneTextures.vbuffer1 = builder.WriteTexture(sceneTextures.vbuffer1, RenderBackendResourceState::RenderTarget);
                    auto sceneDepthTexture = sceneTextures.sceneDepthTexture = builder.WriteTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);

                    builder.BindColorTarget(0, vbuffer0, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);
                    builder.BindColorTarget(1, vbuffer1, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);
                    builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        RenderBackendViewport viewport(0.0f, 0.0f, (float)renderResolution.width, (float)renderResolution.height);
                        commandList.SetViewports(&viewport, 1);

                        RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                        commandList.SetScissors(&scissor, 1);

                        RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::VBufferMeshlet);

                        RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
                        graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
                        graphicsPipelineState.depthStencilState.depthTestEnable = true;
                        graphicsPipelineState.depthStencilState.depthWriteEnable = true;
                        graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

                        for (const auto& drawCallInfo : renderEngine->drawList)
                        {
                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
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
#endif
    }

    void RealTimeRenderer::RenderGBuffer(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        // renderGraph.AddPass(std::format("GBuffer (Compute, {}x{})", renderResolution.width, renderResolution.height), RenderGraphPassFlags::Compute,
        //     [&](RenderGraphBuilder& builder)
        //     {
        //         auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        //
        //         auto vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);
        //         auto vbuffer1 = builder.ReadTexture(sceneTextures.vbuffer1, RenderBackendResourceState::ShaderResource);
        //
        //         auto gbuffer0 = sceneTextures.gbuffer0 = builder.WriteTexture(sceneTextures.gbuffer0, RenderBackendResourceState::UnorderedAccess);
        //         auto gbuffer1 = sceneTextures.gbuffer1 = builder.WriteTexture(sceneTextures.gbuffer1, RenderBackendResourceState::UnorderedAccess);
        //         auto gbuffer2 = sceneTextures.gbuffer2 = builder.WriteTexture(sceneTextures.gbuffer2, RenderBackendResourceState::UnorderedAccess);
        //
        //         return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //         {
        //             RenderBackendShaderArguments shaderArguments = {};
        //             shaderArguments.debugName = "GBuffer";
        //             shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
        //             shaderArguments.BindBuffer(1, renderEngine->geometryBuffer, 0);
        //             shaderArguments.BindBuffer(2, renderEngine->materialBuffer, 0);
        //             shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(vbuffer0)));
        //             shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(vbuffer1)));
        //             shaderArguments.BindTextureUAV(5, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer0)));
        //             shaderArguments.BindTextureUAV(6, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer1)));
        //             shaderArguments.BindTextureUAV(7, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer2)));
        //
        //             uint32 threadGroupCountX = ComputeWorkGroupCount(renderResolution.width, 8);
        //             uint32 threadGroupCountY = ComputeWorkGroupCount(renderResolution.height, 8);
        //             uint32 threadGroupCountZ = 1;
        //
        //             RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::GBuffer);
        //             commandList.Dispatch(
        //                 computeShader,
        //                 shaderArguments,
        //                 threadGroupCountX,
        //                 threadGroupCountY,
        //                 threadGroupCountZ);
        //         };
        //     });
    }

    void RealTimeRenderer::RenderMotionVectors(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        // RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        //
        // renderGraph.AddPass(
        //     std::format("MotionVectors (Compute, {}x{})", renderResolution.width, renderResolution.height),
        //     RenderGraphPassFlags::Compute,
        //     [&](RenderGraphBuilder& builder)
        //     {
        //         RenderGraphTextureHandle vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);
        //         RenderGraphTextureHandle motionVectorTexture = sceneTextures.motionVectorTexture = builder.WriteTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::UnorderedAccess);
        //
        //         return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //         {
        //             uint32 threadGroupCountX = ComputeWorkGroupCount(renderResolution.width, 8);
        //             uint32 threadGroupCountY = ComputeWorkGroupCount(renderResolution.height, 8);
        //             uint32 threadGroupCountZ = 1;
        //
        //             RenderBackendShaderArguments shaderArguments = {};
        //             shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
        //             shaderArguments.BindBuffer(1, renderEngine->geometryBuffer, 0);
        //             shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(vbuffer0)));
        //             shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(motionVectorTexture), 0));
        //
        //             RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::MotionVectors);
        //             commandList.Dispatch(
        //                 computeShader,
        //                 shaderArguments,
        //                 threadGroupCountX,
        //                 threadGroupCountY,
        //                 threadGroupCountZ);
        //         };
        //     });
    }

    void RealTimeRenderer::AddDirectLightingPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle screenSpaceShadowMaskTexture,
        RenderGraphTextureHandle localLightShadowMapAtlas)
    {
        // RenderGraphTextureHandle skyAtmosphereTransmittanceLUT = renderSystem->GetDefaultResources().ImportWhiteDummyTexture2D(renderGraph);
        // if (shouldRenderSkyAtmosphere)
        // {
        //     RealTimeRendererSkyAtmosphereLUTs& skyAtmosphereLUTs = renderGraph.blackboard.Get<RealTimeRendererSkyAtmosphereLUTs>();
        //     skyAtmosphereTransmittanceLUT = skyAtmosphereLUTs.transmittanceLut;
        // }
        //
        // renderGraph.AddPass(
        //     std::format("DirectLighting (Graphics, {}x{})", renderResolution.width, renderResolution.height),
        //     RenderGraphPassFlags::Graphics,
        //     [&](RenderGraphBuilder& builder)
        //     {
        //         RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        //
        //         auto vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);
        //         auto vbuffer1 = builder.ReadTexture(sceneTextures.vbuffer1, RenderBackendResourceState::ShaderResource);
        //         auto gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
        //         auto gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
        //         auto gbuffer2 = builder.ReadTexture(sceneTextures.gbuffer2, RenderBackendResourceState::ShaderResource);
        //         // TODO: which state should be?
        //         auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
        //         //auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);
        //         screenSpaceShadowMaskTexture = builder.ReadTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::ShaderResource);
        //         localLightShadowMapAtlas = builder.ReadTexture(localLightShadowMapAtlas, RenderBackendResourceState::ShaderResource);
        //         skyAtmosphereTransmittanceLUT = builder.ReadTexture(skyAtmosphereTransmittanceLUT, RenderBackendResourceState::ShaderResource);
        //
        //         auto sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);
        //
        //         builder.BindColorTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);
        //         builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);
        //
        //         return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //         {
        //             RenderBackendShaderArguments shaderArguments = {};
        //             shaderArguments.debugName = "DirectLighting";
        //             shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
        //             shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
        //             shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(vbuffer0)));
        //             shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(vbuffer1)));
        //             shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer0)));
        //             shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer1)));
        //             shaderArguments.BindTextureSRV(10, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer2)));
        //             shaderArguments.BindTextureSRV(8, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(screenSpaceShadowMaskTexture)));
        //             shaderArguments.BindTextureSRV(9, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(localLightShadowMapAtlas)));
        //             shaderArguments.BindTextureSRV(15, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(skyAtmosphereTransmittanceLUT)));
        //             shaderArguments.BindBuffer(11, renderEngine->geometryBuffer, 0); // TODO: fix crash when geometryBuffer is null
        //             shaderArguments.BindBuffer(12, renderEngine->materialBuffer, 0);
        //             shaderArguments.BindBuffer(13, renderEngine->lightDataBuffer, 0);
        //             shaderArguments.BindBuffer(14, renderEngine->cubeShadowMapBuffer, 0);
        //             shaderArguments.PushConstants(0, (float)renderEngine->numLights);
        //             //shaderArguments.PushConstants(0, (float)renderEngine->numLights);
        //
        //             // struct PassParameters
        //             // {
        //             //     uint32 numLights;
        //             // };
        //             // PassParameters parameters;
        //             // parameters.numLights = renderEngine->numLights;
        //             // shaderArguments.PushConstantsTest(&parameters, sizeof(PassParameters));
        //
        //             RenderBackendGraphicsPipelineState graphicsPipelineState = {};
        //             graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
        //             graphicsPipelineState.depthStencilState.depthTestEnable = true;
        //             graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
        //             graphicsPipelineState.depthStencilState.depthWriteEnable = false;
        //             graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
        //             graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::One;
        //             graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::One;
        //             graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
        //             graphicsPipelineState.colorBlendState.targetBlends[0].srcAlphaBlendFactor = RenderBackendBlendFactor::One;
        //             graphicsPipelineState.colorBlendState.targetBlends[0].dstAlphaBlendFactor = RenderBackendBlendFactor::One;
        //             graphicsPipelineState.colorBlendState.targetBlends[0].alphaBlendOp = RenderBackendBlendOp::Add;
        //             graphicsPipelineState.colorBlendState.targetBlends[0].colorWriteMask = RenderBackendColorComponentFlags::RGBA;
        //
        //             RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::DirectLighting);
        //             commandList.Draw(
        //                 graphicsShader,
        //                 graphicsPipelineState,
        //                 shaderArguments,
        //                 3, 1, 0, 0,
        //                 RenderBackendPrimitiveTopology::TriangleList);
        //         };
        //     });
    }

    void RealTimeRenderer::AddIndirectLightingDiffusePass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        // renderGraph.AddPass(std::format("IndirectLightingDiffuse (Graphics, {}x{})", renderResolution.width, renderResolution.height), RenderGraphPassFlags::Graphics,
        //     [&](RenderGraphBuilder& builder)
        //     {
        //         auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        //         auto gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
        //         auto gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
        //         auto gbuffer2 = builder.ReadTexture(sceneTextures.gbuffer2, RenderBackendResourceState::ShaderResource);
        //         auto ambientOcclusionTexture = builder.ReadTexture(sceneTextures.ambientOcclusionTexture, RenderBackendResourceState::ShaderResource);
        //         auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);
        //
        //         auto sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);
        //
        //         // TODO: move clear render target to a separate pass
        //         builder.BindColorTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);
        //         builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);
        //
        //         return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //         {
        //             RenderBackendGraphicsPipelineState graphicsPipelineState = {};
        //             graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
        //             graphicsPipelineState.depthStencilState.depthTestEnable = true;
        //             graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
        //             graphicsPipelineState.depthStencilState.depthWriteEnable = false;
        //
        //             RenderBackendShaderArguments shaderArguments = {};
        //             shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
        //             shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer0)));
        //             shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer1)));
        //             shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer2)));
        //             shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(ambientOcclusionTexture)));
        //             shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(renderEngine->irradianceEnvironmentMap));
        //             shaderArguments.BindBuffer(6, renderEngine->irradianceEnvironmentMapSH, 0);
        //
        //             RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::IndirectLightingDiffuse);
        //             commandList.Draw(
        //                 graphicsShader,
        //                 graphicsPipelineState,
        //                 shaderArguments,
        //                 3, 1, 0, 0,
        //                 RenderBackendPrimitiveTopology::TriangleList);
        //         };
        //     });
    }

    void RealTimeRenderer::AddIndirectLightingSpecularPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        // renderGraph.AddPass(std::format("IndirectLightingSpecular (Graphics, {}x{})", renderResolution.width, renderResolution.height), RenderGraphPassFlags::Graphics,
        //     [&](RenderGraphBuilder& builder)
        //     {
        //         auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        //         auto gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
        //         auto gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
        //         auto gbuffer2 = builder.ReadTexture(sceneTextures.gbuffer2, RenderBackendResourceState::ShaderResource);
        //         auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);
        //         auto ambientOcclusionTexture = builder.ReadTexture(sceneTextures.ambientOcclusionTexture, RenderBackendResourceState::ShaderResource);
        //
        //         auto sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);
        //
        //         builder.BindColorTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);
        //         builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);
        //
        //         return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //         {
        //             RenderBackendGraphicsPipelineState graphicsPipelineState = {};
        //             graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
        //             graphicsPipelineState.depthStencilState.depthTestEnable = true;
        //             graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
        //             graphicsPipelineState.depthStencilState.depthWriteEnable = false;
        //             graphicsPipelineState.colorBlendState.targetBlends[0] = additiveColorBlendAttachmentStateRGBA;
        //
        //             RenderBackendShaderArguments shaderArguments = {};
        //             shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
        //             shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer0)));
        //             shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer1)));
        //             shaderArguments.BindTextureSRV(7, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer2)));
        //             shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
        //             shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(ambientOcclusionTexture)));
        //             shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(renderEngine->GetDefaultResources().GetPreIntegratedBrdfLut()));
        //             shaderArguments.BindTextureSRV(6, RenderBackendTextureSRVDesc::Create(renderEngine->filteredEnvironmentMap));
        //
        //             RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::IndirectLightingSpecular);
        //             commandList.Draw(
        //                 graphicsShader,
        //                 graphicsPipelineState,
        //                 shaderArguments,
        //                 3, 1, 0, 0,
        //                 RenderBackendPrimitiveTopology::TriangleList);
        //         };
        //     });
    }
}