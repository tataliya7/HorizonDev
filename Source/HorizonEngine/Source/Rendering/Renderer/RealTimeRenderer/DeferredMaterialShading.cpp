#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::RenderVisibilityBuffer(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        renderGraph.AddPass("VisibilityBuffer", RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                RenderGraphTextureHandle vbuffer0 = sceneTextures.vbuffer0 = builder.WriteTexture(sceneTextures.vbuffer0, RenderBackendResourceState::RenderTarget);
                RenderGraphTextureHandle vbuffer1 = sceneTextures.vbuffer1 = builder.WriteTexture(sceneTextures.vbuffer1, RenderBackendResourceState::RenderTarget);
                RenderGraphTextureHandle sceneDepthTexture = sceneTextures.sceneDepthTexture = builder.WriteTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);

                builder.BindRenderTarget(0, vbuffer0, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);
                builder.BindRenderTarget(1, vbuffer1, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);
                builder.BindDepthStencil(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(renderResolution.width), float(renderResolution.height));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                    commandList.SetScissors(&scissor, 1);

                    DispatchGeometryOpaquePassDrawCommands(commandList);
                };
            });
    }

    void RealTimeRenderer::RenderVisibilityBufferMeshShading(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
#if 0
        renderGraph.AddPass(std::format("VisibilityBuffer"), RenderGraphPassFlags::MeshShading,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                auto vbuffer0 = sceneTextures.vbuffer0 = builder.WriteTexture(sceneTextures.vbuffer0, RenderBackendResourceState::RenderTarget);
                auto vbuffer1 = sceneTextures.vbuffer1 = builder.WriteTexture(sceneTextures.vbuffer1, RenderBackendResourceState::RenderTarget);
                auto sceneDepthTexture = sceneTextures.sceneDepthTexture = builder.WriteTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);

                builder.BindRenderTarget(0, vbuffer0, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);
                builder.BindRenderTarget(1, vbuffer1, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);
                builder.BindDepthStencil(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

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
                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        shaderConstants.BindBuffer(1, renderEngine->geometryBuffer, drawCallInfo.geometryIndex * sizeof(GeometryShaderParameters));
                        shaderConstants.BindBuffer(2, renderEngine->materialBuffer, 0);
                        shaderConstants.PushConstants(0, (float)drawCallInfo.geometryIndex);

                        uint32 meshletCount = 1;
                        commandList.DisptachMesh(
                            graphicsShader,
                            graphicsPipelineState,
                            shaderConstants,
                            meshletCount,
                            1,
                            1,
                            RenderBackendPrimitiveTopology::TriangleList);
                    }
                };
            });
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
        //             RenderBackendShaderConstants shaderConstants = {};
        //             shaderConstants.debugName = "GBuffer";
        //             shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //             shaderConstants.BindBuffer(1, renderEngine->geometryBuffer, 0);
        //             shaderConstants.BindBuffer(2, renderEngine->materialBuffer, 0);
        //             shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0)));
        //             shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer1)));
        //             shaderConstants.BindTextureUAV(5, registry.GetTextureUAVBindlessResourceDescriptorIndexgbuffer0)));
        //             shaderConstants.BindTextureUAV(6, registry.GetTextureUAVBindlessResourceDescriptorIndexgbuffer1)));
        //             shaderConstants.BindTextureUAV(7, registry.GetTextureUAVBindlessResourceDescriptorIndexgbuffer2)));
        //
        //             uint32 threadGroupCountX = ComputeWorkGroupCount(renderResolution.width, 8);
        //             uint32 threadGroupCountY = ComputeWorkGroupCount(renderResolution.height, 8);
        //             uint32 threadGroupCountZ = 1;
        //
        //             RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::GBuffer);
        //             commandList.Dispatch(
        //                 computeShader,
        //                 shaderConstants,
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
         RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

         renderGraph.AddPass(
             std::format("MotionVectors (Compute, {}x{})", renderResolution.width, renderResolution.height),
             RenderGraphPassFlags::Compute,
             [&](RenderGraphBuilder& builder)
             {
                 RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                 RenderGraphTextureHandle vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);
                 RenderGraphTextureHandle motionVectorTexture = sceneTextures.motionVectorTexture = builder.WriteTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::UnorderedAccess);

                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                 {
                     uint32 threadGroupCountX = CeilDiv(renderResolution.width, 8);
                     uint32 threadGroupCountY = CeilDiv(renderResolution.height, 8);
                     uint32 threadGroupCountZ = 1;

                     RenderBackendShaderConstants shaderConstants = {};
                     shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                     //shaderConstants.BindBuffer(1, renderEngine->geometryBuffer, 0);
                     shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                     shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0));
                     shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndex(motionVectorTexture, 0));

                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::MotionVectors);

                     commandList.Dispatch(
                         computeShader,
                         shaderConstants,
                         threadGroupCountX,
                         threadGroupCountY,
                         threadGroupCountZ);
                 };
             });
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
        //             RenderBackendShaderConstants shaderConstants = {};
        //             shaderConstants.debugName = "DirectLighting";
        //             shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //             shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture)));
        //             shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0)));
        //             shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer1)));
        //             shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0)));
        //             shaderConstants.BindTextureSRV(5, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1)));
        //             shaderConstants.BindTextureSRV(10, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer2)));
        //             shaderConstants.BindTextureSRV(8, registry.GetTextureSRVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture)));
        //             shaderConstants.BindTextureSRV(9, registry.GetTextureSRVBindlessResourceDescriptorIndex(localLightShadowMapAtlas)));
        //             shaderConstants.BindTextureSRV(15, registry.GetTextureSRVBindlessResourceDescriptorIndex(skyAtmosphereTransmittanceLUT)));
        //             shaderConstants.BindBuffer(11, renderEngine->geometryBuffer, 0); // TODO: fix crash when geometryBuffer is null
        //             shaderConstants.BindBuffer(12, renderEngine->materialBuffer, 0);
        //             shaderConstants.BindBuffer(13, renderEngine->lightDataBuffer, 0);
        //             shaderConstants.BindBuffer(14, renderEngine->cubeShadowMapBuffer, 0);
        //             shaderConstants.PushConstants(0, (float)renderEngine->numLights);
        //             //shaderConstants.PushConstants(0, (float)renderEngine->numLights);
        //
        //             // struct PassParameters
        //             // {
        //             //     uint32 numLights;
        //             // };
        //             // PassParameters parameters;
        //             // parameters.numLights = renderEngine->numLights;
        //             // shaderConstants.PushConstantsTest(&parameters, sizeof(PassParameters));
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
        //                 shaderConstants,
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
        //             RenderBackendShaderConstants shaderConstants = {};
        //             shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //             shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0)));
        //             shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1)));
        //             shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer2)));
        //             shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(ambientOcclusionTexture)));
        //             shaderConstants.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(renderEngine->irradianceEnvironmentMap));
        //             shaderConstants.BindBuffer(6, renderEngine->irradianceEnvironmentMapSH, 0);
        //
        //             RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::IndirectLightingDiffuse);
        //             commandList.Draw(
        //                 graphicsShader,
        //                 graphicsPipelineState,
        //                 shaderConstants,
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
        //             RenderBackendShaderConstants shaderConstants = {};
        //             shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //             shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0)));
        //             shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1)));
        //             shaderConstants.BindTextureSRV(7, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer2)));
        //             shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture)));
        //             shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(ambientOcclusionTexture)));
        //             shaderConstants.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(renderEngine->GetDefaultResources().GetPreIntegratedBrdfLut()));
        //             shaderConstants.BindTextureSRV(6, RenderBackendTextureSRVDesc::Create(renderEngine->filteredEnvironmentMap));
        //
        //             RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::IndirectLightingSpecular);
        //             commandList.Draw(
        //                 graphicsShader,
        //                 graphicsPipelineState,
        //                 shaderConstants,
        //                 3, 1, 0, 0,
        //                 RenderBackendPrimitiveTopology::TriangleList);
        //         };
        //     });
    }
}