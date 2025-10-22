#include "RasterizationRenderer.h"
#include "AtmosphereRendering.h"

namespace Horizon
{
    void RasterizationRenderer::DispatchVisibilityCulling(
        RenderGraph& renderGraph,
        const SceneView& view)
    {

    }

    void RasterizationRenderer::DispatchOpaqueGeometryPassDrawCommands(RenderBackendCommandList& commandList)
    {
        // const GeometryPassDrawCommandList& drawCommandList = geometryPassDrawCommandLists[uint32(GeometryPassType::Opaque)];
        // const GPUScene* gpuScene = sceneView->scene->GetGPUScene();//drawCommandList.setupJobData.scene->GetGPUScene();
        //
        // RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::VisibilityBufferVS);
        // RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::VisibilityBufferPS);
        //
        // for (uint32 drawCommandIndex = 0; drawCommandIndex < drawCommandList.drawCommandCount; drawCommandIndex++)
        // {
        //     const GeometryPassDrawCommand& drawCommand = drawCommandList.commands[drawCommandIndex];
        //
        //     RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
        //     graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
        //     graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
        //     graphicsPipelineState.depthStencilState.depthTestEnable = true;
        //     graphicsPipelineState.depthStencilState.depthWriteEnable = true;
        //     graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;
        //
        //     commandList.SetStencilReference(drawCommand.stencilReference);
        //
        //     RenderBackendPushConstantValues pushConstantValues = {};
        //     pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //     pushConstantValues.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
        //     pushConstantValues.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
        //
        //     commandList.Draw(
        //         vertexShader,
        //         pixelShader,
        //         graphicsPipelineState,
        //         pushConstantValues,
        //         IndexCountPerMeshlet,
        //         Math::CeilDiv(drawCommand.indexCount, IndexCountPerMeshlet),
        //         0,
        //         0,
        //         drawCommand.topology);
        // }
    }

    void RasterizationRenderer::RenderVisibilityBuffer(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const GeometryPassDrawCommandList& drawCommandList = geometryPassDrawCommandLists[uint32(GeometryPassType::Opaque)];
        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();
        RenderGraphBufferHandle geometryDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryDataBuffer);
        RenderGraphBufferHandle geometryInstanceDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryInstanceDataBuffer);

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        RenderGraphBufferDescription meshletCullingArgumentBufferDesc = RenderGraphBufferDescription::CreateIndirectArguments(sizeof(RenderBackendDispatchIndirectArguments), 1);
        RenderGraphBufferHandle meshletCullingArgumentBuffer = renderGraph.CreateBuffer(meshletCullingArgumentBufferDesc, "VirtualGeometryIndirectArgumentBuffer");

        RenderGraphBufferDescription drawIndirectArgumentBufferDesc = RenderGraphBufferDescription::CreateIndirectArguments(sizeof(RenderBackendDrawIndirectArguments), 1);
        RenderGraphBufferHandle drawIndirectArgumentBuffer = renderGraph.CreateBuffer(drawIndirectArgumentBufferDesc, "VirtualGeometryDrawIndirectArgumentBuffer");

        RenderGraphBufferDescription candidateVisibleMeshletBufferDesc = RenderGraphBufferDescription::CreateByteAddress(sizeof(VisibleMeshletEntry) * MaximumCandidateVisibleMeshletCount);
        RenderGraphBufferHandle candidateVisibleMeshletBuffer = renderGraph.CreateBuffer(candidateVisibleMeshletBufferDesc, "CandidateVisibleMeshletBuffer");

        RenderGraphBufferDescription visibleMeshletBufferDesc = RenderGraphBufferDescription::CreateByteAddress(sizeof(VisibleMeshletEntry) * MaximumVisibleMeshletCount);
        RenderGraphBufferHandle visibleMeshletBuffer = renderGraph.CreateBuffer(visibleMeshletBufferDesc, "VisibleMeshletBuffer");

        RenderGraphBufferDescription visibleMeshletCounterBufferDesc = RenderGraphBufferDescription::CreateByteAddress(sizeof(uint32));
        RenderGraphBufferHandle visibleMeshletCounterBuffer = renderGraph.CreateBuffer(visibleMeshletCounterBufferDesc, "VisibleMeshletCounterBuffer");

        renderGraph.AddPass(
            std::format("ClearVisibleMeshletCounterBuffer ({} bytes)", visibleMeshletCounterBufferDesc.size),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                visibleMeshletCounterBuffer = builder.WriteBuffer(visibleMeshletCounterBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.ClearBufferUAV(resourceRegistry.GetRenderBackendBufferHandle(visibleMeshletCounterBuffer), 0);
                };
            });

        renderGraph.AddPass(
            std::format("VisibilityCullingIndirectArgumentInitialization (Compute, 1x1x1)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                meshletCullingArgumentBuffer = builder.WriteBuffer(meshletCullingArgumentBuffer, RenderBackendResourceState::UnorderedAccess);
                drawIndirectArgumentBuffer = builder.WriteBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VisibilityCullingIndirectArgumentInitialization);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferUAV(0, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(meshletCullingArgumentBuffer));
                    pushConstantValues.BindBufferUAV(1, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(drawIndirectArgumentBuffer));

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        1,
                        1,
                        1);
                };
            });

        const uint32 geometryInstanceCount = gpuScene->geometryInstanceCount;
        if (geometryInstanceCount > 0)
        {
            renderGraph.AddPass(
                std::format("InstanceCulling (Compute)"),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    meshletCullingArgumentBuffer = builder.WriteBuffer(meshletCullingArgumentBuffer, RenderBackendResourceState::UnorderedAccess);

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VirtualGeometryInstanceCulling);

                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(geometryInstanceCount, 64);
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        pushConstantValues.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryDataBuffer));
                        pushConstantValues.BindBufferSRV(2, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryInstanceDataBuffer));
                        pushConstantValues.BindBufferUAV(3, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(meshletCullingArgumentBuffer));
                        pushConstantValues.OverrideShaderConstantValue(4, geometryInstanceCount);

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    };
                });
        }

        RenderGraphTextureHandle previousMinDepthPyramidTexture = renderGraph.ImportExternalTexture(historyFrame.minDepthPyramidTexture, "PreviousMinDepthPyramidTexture");
        const bool skipOcclusionCulling = previousMinDepthPyramidTexture.IsNull();

        renderGraph.AddPass(
            std::format("MeshletCulling (Compute, Indirect)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                meshletCullingArgumentBuffer = builder.ReadBuffer(meshletCullingArgumentBuffer, RenderBackendResourceState::IndirectArgument);

                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, geometryDataBuffer);
                builder.SetBindlessResourceSRV(2, geometryInstanceDataBuffer);
                builder.SetBindlessResourceSRV(3, previousMinDepthPyramidTexture);
                builder.SetBindlessResourceUAV(4, visibleMeshletBuffer);
                builder.SetBindlessResourceUAV(5, visibleMeshletCounterBuffer);
                builder.SetBindlessResourceUAV(6, drawIndirectArgumentBuffer);
                builder.SetShaderConstantValue(7, skipOcclusionCulling);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VirtualGeometryMeshletGroupCulling);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.DispatchIndirect(
                        computeShader,
                        pushConstantValues,
                        resourceRegistry.GetRenderBackendBufferHandle(meshletCullingArgumentBuffer),
                        0);
                };
            });

        renderGraph.AddPass(
            "VisibilityBuffer (Graphics, Indirect)",
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                drawIndirectArgumentBuffer = builder.ReadBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);
                visibleMeshletBuffer = builder.ReadBuffer(visibleMeshletBuffer, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle vbuffer0 = intermediateResources.vbuffer0;
                RenderGraphTextureHandle vbuffer1 = intermediateResources.vbuffer1;
                RenderGraphTextureHandle depthTexture = intermediateResources.depthTexture;

                builder.SetRenderTargetBinding(0, vbuffer0, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Store);
                builder.SetRenderTargetBinding(1, vbuffer1, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Store);
                builder.SetDepthStencilBinding(depthTexture,
                    RenderBackendRenderPassLoadOperation::Clear,
                    RenderBackendRenderPassStoreOperation::Store,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthWrite_StencilNoAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(renderResolution.width), float(renderResolution.height));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::VisibilityBufferVS);
                    RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::VisibilityBufferPS);

                    {
                        RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
                        graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
                        graphicsPipelineState.depthStencilState.depthTestEnable = true;
                        graphicsPipelineState.depthStencilState.depthWriteEnable = true;
                        graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        pushConstantValues.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryDataBuffer));
                        pushConstantValues.BindBufferSRV(2, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryInstanceDataBuffer));
                        pushConstantValues.BindBufferSRV(3, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(visibleMeshletBuffer));

                        commandList.DrawIndirect(
                            vertexShader,
                            pixelShader,
                            graphicsPipelineState,
                            pushConstantValues,
                            resourceRegistry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer),
                            0,
                            1,
                            RenderBackendPrimitiveTopology::TriangleList);
                    }
                };
            });

        intermediateResources.visibleMeshletBuffer = visibleMeshletBuffer;

        if (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::Depth)
        {
            debugVisualizationCallback = std::bind(&RasterizationRenderer::DispatchDepthDebugVisualization, this, std::placeholders::_1, std::placeholders::_2);
        }

        if (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::WorldSpaceNormal)
        {
            debugVisualizationCallback = std::bind(&RasterizationRenderer::DispatchWorldSpaceNormalDebugVisualization, this, std::placeholders::_1, std::placeholders::_2);
        }

        if (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::MotionVectors)
        {
            debugVisualizationCallback = std::bind(&RasterizationRenderer::DispatchMotionVectorDebugVisualization, this, std::placeholders::_1, std::placeholders::_2);
        }

        const bool isVirtualGeometryDebugVisualizationEnabled =
            (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::MeshletID) ||
            (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::PrimitiveID) ||
            (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::MaterialID);
        if (isVirtualGeometryDebugVisualizationEnabled)
        {
            debugVisualizationCallback = std::bind(&RasterizationRenderer::DispatchVirtualGeometryDebugVisualization, this, std::placeholders::_1, std::placeholders::_2);
        }
    }

    void RasterizationRenderer::RenderVisibilityBufferMeshShading(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
#if 0
        renderGraph.AddPass(
            std::format("VisibilityBuffer"),
            RenderGraphPassFlags::MeshShading,
            [&](RenderGraphBuilder& builder)
            {
                auto& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererSceneTextures>();

                auto vbuffer0 = intermediateResources.vbuffer0;
                auto vbuffer1 = intermediateResources.vbuffer1;
                auto sceneDepthTexture = intermediateResources.sceneDepthTexture;

                builder.SetRenderTargetBinding(0, vbuffer0, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Load);
                builder.SetRenderTargetBinding(1, vbuffer1, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Load);
                builder.SetDepthStencilBinding(sceneDepthTexture,
                    RenderBackendRenderPassLoadOperation::Clear,
                    RenderBackendRenderPassStoreOperation::Load,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthWrite_StencilNoAccess);


                {
                    RenderBackendViewport viewport(0.0f, 0.0f, (float)renderResolution.width, (float)renderResolution.height);
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendShaderHandle graphicsShader = shaderRepository->GetShader(ShaderID::VBufferMeshlet);

                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
                    graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

                    for (const auto& drawCallInfo : renderEngine->drawList)
                    {
                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        pushConstantValues.BindBuffer(1, renderEngine->geometryBuffer, drawCallInfo.geometryIndex * sizeof(GeometryShaderParameters));
                        pushConstantValues.BindBuffer(2, renderEngine->materialBuffer, 0);
                        pushConstantValues.PushConstants(0, (float)drawCallInfo.geometryIndex);

                        uint32 meshletCount = 1;
                        commandList.DisptachMesh(
                            graphicsShader,
                            graphicsPipelineState,
                            pushConstantValues,
                            meshletCount,
                            1,
                            1,
                            RenderBackendPrimitiveTopology::TriangleList);
                    }
                };
            });
#endif
    }

    void RasterizationRenderer::RenderGBuffer(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();
        RenderGraphBufferHandle geometryDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryDataBuffer);
        RenderGraphBufferHandle geometryInstanceDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryInstanceDataBuffer);

        renderGraph.AddPass(
            std::format("GBuffer (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

                RenderGraphBufferHandle visibleMeshletBuffer = builder.ReadBuffer(intermediateResources.visibleMeshletBuffer, RenderBackendResourceState::ShaderResource);

                RenderGraphTextureHandle vbuffer0 = builder.ReadTexture(intermediateResources.vbuffer0, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle vbuffer1 = builder.ReadTexture(intermediateResources.vbuffer1, RenderBackendResourceState::ShaderResource);

                RenderGraphTextureHandle gbuffer0 = intermediateResources.gbuffer0 = builder.WriteTexture(intermediateResources.gbuffer0, RenderBackendResourceState::UnorderedAccess);
                RenderGraphTextureHandle gbuffer1 = intermediateResources.gbuffer1 = builder.WriteTexture(intermediateResources.gbuffer1, RenderBackendResourceState::UnorderedAccess);
                RenderGraphTextureHandle gbuffer2 = intermediateResources.gbuffer2 = builder.WriteTexture(intermediateResources.gbuffer2, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryDataBuffer));
                    pushConstantValues.BindBufferSRV(2, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryInstanceDataBuffer));
                    pushConstantValues.BindBufferSRV(3, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(visibleMeshletBuffer));
                    pushConstantValues.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0));
                    pushConstantValues.BindTextureSRV(5, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer1));
                    pushConstantValues.BindTextureUAV(6, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(gbuffer0, 0));
                    pushConstantValues.BindTextureUAV(7, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(gbuffer1, 0));
                    pushConstantValues.BindTextureUAV(8, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(gbuffer2, 0));

                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::GBuffer);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });
    }

    void RasterizationRenderer::RenderMotionVectors(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();
        RenderGraphBufferHandle geometryDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryDataBuffer);
        RenderGraphBufferHandle geometryInstanceDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryInstanceDataBuffer);

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        renderGraph.AddPass(
            std::format("ComputeMotionVectors (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, geometryDataBuffer);
                builder.SetBindlessResourceSRV(2, geometryInstanceDataBuffer);
                builder.SetBindlessResourceSRV(3, intermediateResources.visibleMeshletBuffer);
                builder.SetBindlessResourceSRV(4, intermediateResources.depthTexture);
                builder.SetBindlessResourceSRV(5, intermediateResources.vbuffer0);
                builder.SetBindlessResourceSRV(6, intermediateResources.vbuffer1);
                builder.SetBindlessResourceUAV(7, intermediateResources.motionVectorTexture, 0);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::ComputeMotionVectors);

                uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, 8);
                uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, 8);
                uint32 threadGroupCountZ = 1;

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });
    }

    void RasterizationRenderer::AddDirectLightingPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle localLightShadowMapAtlas)
    {
        RenderGraphTextureHandle skyAtmosphereTransmittanceLUT = defaultResources->ImportWhiteDummyTexture2D(renderGraph);
        if (renderFeatures.enableSkyAtmosphereRendering)
        {
            RasterizationRendererSkyAtmosphereLUTs& skyAtmosphereLUTs = renderGraph.blackboard.Get<RasterizationRendererSkyAtmosphereLUTs>();
            skyAtmosphereTransmittanceLUT = skyAtmosphereLUTs.transmittanceLut;
        }

        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();
        RenderGraphBufferHandle geometryDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryDataBuffer);
        RenderGraphBufferHandle geometryInstanceDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryInstanceDataBuffer);
        RenderGraphBufferHandle distantLightDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentDistantLightDataBuffer);
        RenderGraphBufferHandle localLightDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentLocalLightDataBuffer);

        renderGraph.AddPass(
            std::format("DirectLighting (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();
                RasterizationRendererLightGridData& lightGridData = renderGraph.blackboard.Get<RasterizationRendererLightGridData>();

                RenderGraphTextureHandle vbuffer0 = builder.ReadTexture(intermediateResources.vbuffer0, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle vbuffer1 = builder.ReadTexture(intermediateResources.vbuffer1, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer0 = builder.ReadTexture(intermediateResources.gbuffer0, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(intermediateResources.gbuffer1, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer2 = builder.ReadTexture(intermediateResources.gbuffer2, RenderBackendResourceState::ShaderResource);
                // TODO: which state should be?
                //RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(intermediateResources.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = intermediateResources.depthTexture;
                RenderGraphTextureHandle screenSpaceShadowMaskTexture = builder.ReadTexture(intermediateResources.shadowMaskTexture, RenderBackendResourceState::ShaderResource);
                localLightShadowMapAtlas = builder.ReadTexture(localLightShadowMapAtlas, RenderBackendResourceState::ShaderResource);
                skyAtmosphereTransmittanceLUT = builder.ReadTexture(skyAtmosphereTransmittanceLUT, RenderBackendResourceState::ShaderResource);

                RenderGraphBufferHandle lightGridCellDataBuffer = builder.ReadBuffer(lightGridData.lightGridCellDataBuffer, RenderBackendResourceState::ShaderResource);
                RenderGraphBufferHandle lightGridLightListBuffer = builder.ReadBuffer(lightGridData.lightGridLightListBuffer, RenderBackendResourceState::ShaderResource);

                RenderGraphTextureHandle sceneColorTexture = intermediateResources.colorTexture;

                builder.SetRenderTargetBinding(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);
                builder.SetDepthStencilBinding(sceneDepthTexture,
                    RenderBackendRenderPassLoadOperation::Load,
                    RenderBackendRenderPassStoreOperation::Store,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthReadOnly_StencilNoAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryDataBuffer));  // TODO: fix crash when geometryBuffer is null
                    pushConstantValues.BindBufferSRV(2, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryInstanceDataBuffer));
                    pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    pushConstantValues.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0));
                    pushConstantValues.BindTextureSRV(5, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer1));
                    pushConstantValues.BindTextureSRV(6, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                    pushConstantValues.BindTextureSRV(7, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                    pushConstantValues.BindTextureSRV(8, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer2));
                    pushConstantValues.BindTextureSRV(9, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(skyAtmosphereTransmittanceLUT));
                    pushConstantValues.BindTextureSRV(10, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture));
                    pushConstantValues.BindTextureSRV(11, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(localLightShadowMapAtlas));
                    pushConstantValues.BindBufferSRV(12, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(distantLightDataBuffer));
                    pushConstantValues.BindBufferSRV(13, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(localLightDataBuffer));
                    pushConstantValues.BindBufferSRV(14, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(lightGridCellDataBuffer));
                    pushConstantValues.BindBufferSRV(15, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(lightGridLightListBuffer));

                    // TODO
                    pushConstantValues.OverrideShaderConstantValue(16, lightGridData.lightGridInfo.lightGridSizeX);
                    pushConstantValues.OverrideShaderConstantValue(17, lightGridData.lightGridInfo.lightGridSizeY);
                    pushConstantValues.OverrideShaderConstantValue(18, lightGridData.lightGridInfo.lightGridSizeZ);

                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0] = RenderBackendColorBlendAttachmentState::Additive;

                    RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::DrawFullscreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::DirectLighting);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        pushConstantValues,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }

    void RasterizationRenderer::AddIndirectLightingDiffusePass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        renderGraph.AddPass(
            std::format("IndirectDiffuseComposition (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();
                RenderGraphTextureHandle gbuffer0 = builder.ReadTexture(intermediateResources.gbuffer0, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(intermediateResources.gbuffer1, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer2 = builder.ReadTexture(intermediateResources.gbuffer2, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = intermediateResources.depthTexture;
                RenderGraphBufferHandle irradianceEnvironmentMapBuffer = builder.ReadBuffer(intermediateResources.irradianceEnvironmentMapBuffer, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle ambientOcclusionTexture = builder.ReadTexture(intermediateResources.ambientOcclusionTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle indirectDiffuseTexture = builder.ReadTexture(intermediateResources.indirectDiffuseTexture, RenderBackendResourceState::ShaderResource);

                RenderGraphTextureHandle sceneColorTexture = intermediateResources.colorTexture;

                builder.SetRenderTargetBinding(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);
                builder.SetDepthStencilBinding(sceneDepthTexture,
                    RenderBackendRenderPassLoadOperation::Load,
                    RenderBackendRenderPassStoreOperation::Store,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthReadOnly_StencilNoAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0] = RenderBackendColorBlendAttachmentState::Additive;
                    //  graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
                    //  graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::One;
                    //  graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::Src1Color;
                    //  graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
                    //  graphicsPipelineState.colorBlendState.targetBlends[0].srcAlphaBlendFactor = RenderBackendBlendFactor::One;
                    //  graphicsPipelineState.colorBlendState.targetBlends[0].dstAlphaBlendFactor = RenderBackendBlendFactor::Src1Alpha;
                    //  graphicsPipelineState.colorBlendState.targetBlends[0].alphaBlendOp = RenderBackendBlendOp::Add;
                    //  graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::RGBA;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                    pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer2));
                    pushConstantValues.BindBufferSRV(4, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(irradianceEnvironmentMapBuffer));
                    pushConstantValues.BindBufferSRV(5, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(view.scene->irradianceEnvironmentMapTexture));
                    pushConstantValues.BindTextureSRV(6, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(ambientOcclusionTexture));
                    pushConstantValues.BindTextureSRV(7, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(indirectDiffuseTexture));

                    RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::DrawFullscreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::IndirectDiffuseComposition);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        pushConstantValues,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }

    void RasterizationRenderer::AddIndirectLightingSpecularPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RenderBackendTextureHandle environmentBrdfLutTexture = defaultResources->GetEnvironmentBrdfLutTexture();

        renderGraph.AddPass(
            std::format("IndirectSpecularComposition (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();
                RenderGraphTextureHandle gbuffer0 = builder.ReadTexture(intermediateResources.gbuffer0, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(intermediateResources.gbuffer1, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer2 = builder.ReadTexture(intermediateResources.gbuffer2, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = intermediateResources.depthTexture;
                RenderGraphTextureHandle indirectSpecularTexture = builder.ReadTexture(intermediateResources.indirectSpecularTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle ambientOcclusionTexture = builder.ReadTexture(intermediateResources.ambientOcclusionTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle convolvedEnvironmentMapTexture = builder.ReadTexture(intermediateResources.convolvedEnvironmentMapTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneColorTexture = intermediateResources.colorTexture;

                builder.SetRenderTargetBinding(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);
                builder.SetDepthStencilBinding(sceneDepthTexture,
                    RenderBackendRenderPassLoadOperation::Load,
                    RenderBackendRenderPassStoreOperation::Store,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthReadOnly_StencilNoAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0] = RenderBackendColorBlendAttachmentState::Additive;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                    pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer2));
                    pushConstantValues.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    pushConstantValues.BindTextureSRV(5, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(indirectSpecularTexture));
                    pushConstantValues.BindTextureSRV(6, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(ambientOcclusionTexture));
                    pushConstantValues.BindTextureSRV(7, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentBrdfLutTexture));
                    pushConstantValues.BindTextureSRV(8, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(convolvedEnvironmentMapTexture));

                    RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::DrawFullscreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::IndirectSpecularComposition);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        pushConstantValues,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }

    RenderGraphTextureHandle RasterizationRenderer::DispatchVirtualGeometryDebugVisualization(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();
        RenderGraphBufferHandle geometryDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryDataBuffer);
        RenderGraphBufferHandle geometryInstanceDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryInstanceDataBuffer);

        const RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");

        uint32 virtualGeometryDebugVisualizationMode = 0;
        if (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::MeshletID)
        {
            virtualGeometryDebugVisualizationMode = 1;
        }
        else if (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::PrimitiveID)
        {
            virtualGeometryDebugVisualizationMode = 2;
        }
        else if (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::MaterialID)
        {
            virtualGeometryDebugVisualizationMode = 3;
        }

        renderGraph.AddPass(
            std::format("VirtualGeometryDebugVisualization (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphBufferHandle visibleMeshletBuffer = builder.ReadBuffer(intermediateResources.visibleMeshletBuffer, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle vbuffer0 = builder.ReadTexture(intermediateResources.vbuffer0, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryDataBuffer));
                    pushConstantValues.BindBufferSRV(2, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryInstanceDataBuffer));
                    pushConstantValues.BindBufferSRV(3, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(visibleMeshletBuffer));
                    pushConstantValues.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0));
                    pushConstantValues.BindTextureUAV(5, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));
                    pushConstantValues.OverrideShaderConstantValue(6, virtualGeometryDebugVisualizationMode);

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VirtualGeometryDebugVisualization);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RasterizationRenderer::DispatchDepthDebugVisualization(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");
        //RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(view.targetTexture->GetDesc(), "VisualizeDepthTexture");

        renderGraph.AddPass(
            std::format("VisualizePrimitiveID (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VisualizeDepth);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RasterizationRenderer::DispatchWorldSpaceNormalDebugVisualization(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");
        //RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(view.targetTexture->GetDesc(), "VisualizeWorldSpaceNormalTexture");

        renderGraph.AddPass(
            std::format("VisualizeWorldSpaceNormal (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle gbuffer0 = builder.ReadTexture(intermediateResources.gbuffer0, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VisualizeWorldSpaceNormal);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RasterizationRenderer::DispatchMotionVectorDebugVisualization(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");
        //RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(view.targetTexture->GetDesc(), "VisualizeMotionVectorsTexture");

        renderGraph.AddPass(
            std::format("VisualizeMotionVectors (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle motionVectorTexture = builder.ReadTexture(intermediateResources.motionVectorTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(motionVectorTexture));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VisualizeMotionVectors);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RasterizationRenderer::AddDebugDrawPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        RenderGraphTextureHandle sceneDepthTexture)
    {
        uint32 newDebugDrawLinesVertexBufferSize = (uint32)debugDrawLinesVertices.size() * sizeof(Vector3f);

        if (newDebugDrawLinesVertexBufferSize == 0)
        {
            return sceneColorTexture;
        }

        RenderBackendBufferHandle& debugDrawLinesVertexUploadBuffer = debugDrawLinesVertexUploadBuffers[currentPerFrameDataBufferIndex];
        RenderBackendBufferHandle& debugDrawLinesVertexBuffer = debugDrawLinesVertexBuffers[currentPerFrameDataBufferIndex];
        uint32& debugDrawLinesVertexBufferSize = debugDrawLinesVertexBufferSizes[currentPerFrameDataBufferIndex];

        if (!debugDrawLinesVertexBuffer)
        {
            RenderBackendBufferDescription bufferDesc = RenderBackendBufferDescription::CreateByteAddress(newDebugDrawLinesVertexBufferSize);
            debugDrawLinesVertexBuffer = renderBackend->CreateBuffer(&bufferDesc, nullptr, "DebugDrawLinesVertexBuffer");

            RenderBackendBufferDescription debugDrawLinesVertexUploadBufferDesc = RenderBackendBufferDescription::CreateUpload(newDebugDrawLinesVertexBufferSize);
            debugDrawLinesVertexUploadBuffer = renderBackend->CreateBuffer(&debugDrawLinesVertexUploadBufferDesc, nullptr, "DebugDrawLinesVertexUploadBuffer");
        }
        else if (debugDrawLinesVertexBufferSize < newDebugDrawLinesVertexBufferSize)
        {
            renderBackend->ResizeBuffer(debugDrawLinesVertexBuffer, newDebugDrawLinesVertexBufferSize);
            renderBackend->ResizeBuffer(debugDrawLinesVertexUploadBuffer, newDebugDrawLinesVertexBufferSize);
        }
        debugDrawLinesVertexBufferSize = newDebugDrawLinesVertexBufferSize;

        if (debugDrawLinesVertexBuffer && debugDrawLinesVertexBufferSize > 0)
        {
            renderBackend->UpdateBuffer(debugDrawLinesVertexUploadBuffer, 0, debugDrawLinesVertices.data(), debugDrawLinesVertexBufferSize);

            renderGraph.AddPass(
                std::format("UpdateDebugDrawLinesVertexBuffer (Copy, {} bytes)", debugDrawLinesVertexBufferSize),
                RenderGraphPassFlags::Copy,
                [&](RenderGraphBuilder& builder)
                {
                    //debugDrawLinesVertexBuffer = builder.WriteBuffer(debugDrawLinesVertexBuffer, RenderBackendResourceState::CopyDst);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        commandList.CopyBuffer(
                            debugDrawLinesVertexUploadBuffer,
                            0,
                            debugDrawLinesVertexBuffer,
                            0,
                            debugDrawLinesVertexBufferSize);
                    };
                });

        }

        renderGraph.AddPass(
            std::format("DebugDrawPass (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetRenderTargetBinding(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);
                builder.SetDepthStencilBinding(
                    sceneDepthTexture,
                    RenderBackendRenderPassLoadOperation::Load,
                    RenderBackendRenderPassStoreOperation::Store,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthWrite_StencilNoAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    // TODO: MSAA
                    // TODO: Remove pixel shader
                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.rasterizationState.lineWidth = 2.0f;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(debugDrawLinesVertexBuffer));

                    RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::DebugDrawVS);
                    RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::DebugDrawPS);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        pushConstantValues,
                        (uint32)debugDrawLinesVertices.size(),
                        1,
                        0,
                        0,
                        RenderBackendPrimitiveTopology::LineList);
                };
            });

        return sceneColorTexture;
    }
}