#include "RealTimeRenderer.h"
#include "SkyAtmosphereRendering.h"

namespace Horizon
{
    void RealTimeRenderer::DispatchOpaqueGeometryPassDrawCommands(RenderBackendCommandList& commandList)
    {
        const GeometryPassDrawCommandList& drawCommandList = geometryPassDrawCommandLists[uint32(GeometryPassType::Opaque)];
        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();//drawCommandList.setupJobData.scene->GetGPUScene();

        RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::VisibilityBufferVS);
        RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::VisibilityBufferPS);

        for (uint32 drawCommandIndex = 0; drawCommandIndex < drawCommandList.drawCommandCount; drawCommandIndex++)
        {
            const GeometryPassDrawCommand& drawCommand = drawCommandList.commands[drawCommandIndex];

            RenderBackendGraphicsPipelineState graphicsPipelineState = {};
            graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
            graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
            graphicsPipelineState.depthStencilState.depthTestEnable = true;
            graphicsPipelineState.depthStencilState.depthWriteEnable = true;
            graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

            commandList.SetStencilReference(drawCommand.stencilReference);

            RenderBackendShaderConstants shaderConstants = {};
            shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
            shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
            shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));

            commandList.DrawIndexed(
                vertexShader,
                pixelShader,
                graphicsPipelineState,
                shaderConstants,
                drawCommand.indexBuffer,
                drawCommand.indexCount,
                drawCommand.instanceCount,
                drawCommand.firstIndex,
                0, // TODO
                drawCommand.firstInstance,
                drawCommand.topology);
        }
    }

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

                    DispatchOpaqueGeometryPassDrawCommands(commandList);
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
        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();

         renderGraph.AddPass(std::format("GBuffer (Compute, {}x{})", renderResolution.width, renderResolution.height), RenderGraphPassFlags::Compute,
             [&](RenderGraphBuilder& builder)
             {
                 RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                 RenderGraphTextureHandle vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);
                 RenderGraphTextureHandle vbuffer1 = builder.ReadTexture(sceneTextures.vbuffer1, RenderBackendResourceState::ShaderResource);

                 RenderGraphTextureHandle gbuffer0 = sceneTextures.gbuffer0 = builder.WriteTexture(sceneTextures.gbuffer0, RenderBackendResourceState::UnorderedAccess);
                 RenderGraphTextureHandle gbuffer1 = sceneTextures.gbuffer1 = builder.WriteTexture(sceneTextures.gbuffer1, RenderBackendResourceState::UnorderedAccess);
                 RenderGraphTextureHandle gbuffer2 = sceneTextures.gbuffer2 = builder.WriteTexture(sceneTextures.gbuffer2, RenderBackendResourceState::UnorderedAccess);

                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                 {
                     RenderBackendShaderConstants shaderConstants = {};
                     shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                     shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
                     shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
                     shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0));
                     shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer1));
                     shaderConstants.BindTextureUAV(5, registry.GetTextureUAVBindlessResourceDescriptorIndex(gbuffer0, 0));
                     shaderConstants.BindTextureUAV(6, registry.GetTextureUAVBindlessResourceDescriptorIndex(gbuffer1, 0));
                     shaderConstants.BindTextureUAV(7, registry.GetTextureUAVBindlessResourceDescriptorIndex(gbuffer2, 0));

                     uint32 threadGroupCountX = CeilDiv(renderResolution.width, 8);
                     uint32 threadGroupCountY = CeilDiv(renderResolution.height, 8);
                     uint32 threadGroupCountZ = 1;

                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::GBuffer);

                     commandList.Dispatch(
                         computeShader,
                         shaderConstants,
                         threadGroupCountX,
                         threadGroupCountY,
                         threadGroupCountZ);
                 };
             });
    }

    void RealTimeRenderer::RenderMotionVectors(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
         renderGraph.AddPass(
             std::format("MotionVectors (Compute, {}x{})", renderResolution.width, renderResolution.height),
             RenderGraphPassFlags::Compute,
             [&](RenderGraphBuilder& builder)
             {
                 RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

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
        //renderGraph.AddPass(
        //    std::format("DirectLighting (Graphics, {}x{})", renderResolution.width, renderResolution.height),
        //    RenderGraphPassFlags::Graphics,
        //    [&](RenderGraphBuilder& builder)
        //    {
        //        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        //        RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);
        //        RenderGraphTextureHandle sceneDepthTexture = sceneTextures.sceneDepthTexture = builder.WriteTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);

        //        builder.BindRenderTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);
        //        builder.BindDepthStencil(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

        //        return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //        {
        //            RenderBackendViewport viewport(0.0f, 0.0f, float(renderResolution.width), float(renderResolution.height));
        //            commandList.SetViewports(&viewport, 1);

        //            RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
        //            commandList.SetScissors(&scissor, 1);
        //        };
        //    });

         RenderGraphTextureHandle skyAtmosphereTransmittanceLUT = defaultResources->ImportWhiteDummyTexture2D(renderGraph);
         if (IsSkyAtmosphereRenderingEnabled())
         {
             RealTimeRendererSkyAtmosphereLUTs& skyAtmosphereLUTs = renderGraph.blackboard.Get<RealTimeRendererSkyAtmosphereLUTs>();
             skyAtmosphereTransmittanceLUT = skyAtmosphereLUTs.transmittanceLut;
         }

         const GPUScene* gpuScene = sceneView->scene->GetGPUScene();

         renderGraph.AddPass(
             std::format("DirectLighting (Graphics, {}x{})", renderResolution.width, renderResolution.height),
             RenderGraphPassFlags::Graphics,
             [&](RenderGraphBuilder& builder)
             {
                 RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                 RenderGraphTextureHandle vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);
                 RenderGraphTextureHandle vbuffer1 = builder.ReadTexture(sceneTextures.vbuffer1, RenderBackendResourceState::ShaderResource);
                 RenderGraphTextureHandle gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
                 RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
                 RenderGraphTextureHandle gbuffer2 = builder.ReadTexture(sceneTextures.gbuffer2, RenderBackendResourceState::ShaderResource);
                 // TODO: which state should be?
                 RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                 //auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);
                 screenSpaceShadowMaskTexture = builder.ReadTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::ShaderResource);
                 localLightShadowMapAtlas = builder.ReadTexture(localLightShadowMapAtlas, RenderBackendResourceState::ShaderResource);
                 skyAtmosphereTransmittanceLUT = builder.ReadTexture(skyAtmosphereTransmittanceLUT, RenderBackendResourceState::ShaderResource);

                 RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                 builder.BindRenderTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);
                 builder.BindDepthStencil(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);

                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                 {
                     RenderBackendShaderConstants shaderConstants = {};
                     shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                     shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));  // TODO: fix crash when geometryBuffer is null
                     shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
                     shaderConstants.BindBufferSRV(3, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->lightDataBuffer));
                     shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                     shaderConstants.BindTextureSRV(5, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0));
                     shaderConstants.BindTextureSRV(6, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer1));
                     shaderConstants.BindTextureSRV(7, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                     shaderConstants.BindTextureSRV(8, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                     shaderConstants.BindTextureSRV(9, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer2));
                     shaderConstants.BindTextureSRV(10, registry.GetTextureSRVBindlessResourceDescriptorIndex(skyAtmosphereTransmittanceLUT));

                     shaderConstants.BindBufferSRV(11, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->lightDataBuffer));
                     shaderConstants.BindTextureSRV(12, registry.GetTextureSRVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture));
                     shaderConstants.BindTextureSRV(13, registry.GetTextureSRVBindlessResourceDescriptorIndex(localLightShadowMapAtlas));

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
                     graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::RGBA;

                     RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::FullScreenQuadVS);
                     RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::DirectLighting);

                     commandList.Draw(
                         vertexShader,
                         pixelShader,
                         graphicsPipelineState,
                         shaderConstants,
                         3, 1, 0, 0,
                         RenderBackendPrimitiveTopology::TriangleList);
                 };
             });
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