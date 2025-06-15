#include "RealTimeRenderer.h"
#include "AtmosphereRendering.h"

namespace Horizon
{
    void RealTimeRenderer::DispatchVisibilityCulling(
        RenderGraph& renderGraph,
        const SceneView& view)
    {

    }

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
            shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
            shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
            shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
            shaderConstants.BindBufferSRV(3, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));

            commandList.Draw(
                vertexShader,
                pixelShader,
                graphicsPipelineState,
                shaderConstants,
                IndexCountPerMeshlet,
                Math::CeilDiv(drawCommand.indexCount, IndexCountPerMeshlet),
                0,
                0,
                drawCommand.topology);
        }
    }

    struct VisibleMeshletEntry
    {
        uint32 geometryInstanceID;
        uint32 meshletID;
    };

    static constexpr uint32 MaximumVisibleMeshletCount = 1048576;
    static constexpr uint32 MaximumCandidateVisibleMeshletCount = 4 * 1048576;

    void RealTimeRenderer::RenderVisibilityBuffer(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const GeometryPassDrawCommandList& drawCommandList = geometryPassDrawCommandLists[uint32(GeometryPassType::Opaque)];
        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();//drawCommandList.setupJobData.scene->GetGPUScene();

        RenderGraphBufferDesc meshletCullingArgumentBufferDesc = RenderGraphBufferDesc::CreateIndirectArguments(sizeof(RenderBackendDispatchIndirectArguments), 1);
        RenderGraphBufferHandle meshletCullingArgumentBuffer = renderGraph.CreateBuffer(meshletCullingArgumentBufferDesc, "VirtualGeometryIndirectArgumentBuffer");

        RenderGraphBufferDesc drawIndirectArgumentBufferDesc = RenderGraphBufferDesc::CreateIndirectArguments(sizeof(RenderBackendDrawIndirectArguments), 1);
        RenderGraphBufferHandle drawIndirectArgumentBuffer = renderGraph.CreateBuffer(drawIndirectArgumentBufferDesc, "VirtualGeometryDrawIndirectArgumentBuffer");

        RenderGraphBufferDesc candidateVisibleMeshletBufferDesc = RenderGraphBufferDesc::CreateByteAddress(sizeof(VisibleMeshletEntry) * MaximumCandidateVisibleMeshletCount);
        RenderGraphBufferHandle candidateVisibleMeshletBuffer = renderGraph.CreateBuffer(candidateVisibleMeshletBufferDesc, "CandidateVisibleMeshletBuffer");

        RenderGraphBufferDesc visibleMeshletBufferDesc = RenderGraphBufferDesc::CreateByteAddress(sizeof(VisibleMeshletEntry) * MaximumVisibleMeshletCount);
        RenderGraphBufferHandle visibleMeshletBuffer = renderGraph.CreateBuffer(visibleMeshletBufferDesc, "VisibleMeshletBuffer");

        RenderGraphBufferDesc visibleMeshletCounterBufferDesc = RenderGraphBufferDesc::CreateByteAddress(sizeof(uint32));
        RenderGraphBufferHandle visibleMeshletCounterBuffer = renderGraph.CreateBuffer(visibleMeshletCounterBufferDesc, "VisibleMeshletCounterBuffer");

        renderGraph.AddPass(
            std::format("ClearVisibleMeshletCounterBuffer ({} bytes)", visibleMeshletCounterBufferDesc.size),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                visibleMeshletCounterBuffer = builder.WriteBuffer(visibleMeshletCounterBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.ClearBufferUAV(registry.GetRenderBackendBufferHandle(visibleMeshletCounterBuffer), 0);
                };
            });

        renderGraph.AddPass(
            std::format("VisibilityCullingIndirectArgumentInitialization (Compute, 1x1x1)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                meshletCullingArgumentBuffer = builder.WriteBuffer(meshletCullingArgumentBuffer, RenderBackendResourceState::UnorderedAccess);
                drawIndirectArgumentBuffer = builder.WriteBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = 1;
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferUAV(0, registry.GetBufferUAVBindlessResourceDescriptorIndex(meshletCullingArgumentBuffer));
                    shaderConstants.BindBufferUAV(1, registry.GetBufferUAVBindlessResourceDescriptorIndex(drawIndirectArgumentBuffer));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisibilityCullingIndirectArgumentInitialization);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("Instance Culling (Compute)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                meshletCullingArgumentBuffer = builder.WriteBuffer(meshletCullingArgumentBuffer, RenderBackendResourceState::UnorderedAccess);
                visibleMeshletBuffer = builder.WriteBuffer(visibleMeshletBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(gpuScene->geometryInstanceCount, 64);
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
                    shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
                    shaderConstants.BindBufferUAV(3, registry.GetBufferUAVBindlessResourceDescriptorIndex(meshletCullingArgumentBuffer));
                    shaderConstants.BindScalar(4, gpuScene->geometryInstanceCount);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VirtualGeometryInstanceCulling);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("Meshlet Culling (Compute)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                visibleMeshletBuffer = builder.WriteBuffer(visibleMeshletBuffer, RenderBackendResourceState::UnorderedAccess);
                visibleMeshletCounterBuffer = builder.WriteBuffer(visibleMeshletCounterBuffer, RenderBackendResourceState::UnorderedAccess);
                drawIndirectArgumentBuffer = builder.WriteBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(2623, 64);
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
                    shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
                    shaderConstants.BindBufferUAV(3, registry.GetBufferUAVBindlessResourceDescriptorIndex(visibleMeshletBuffer));
                    shaderConstants.BindBufferUAV(4, registry.GetBufferUAVBindlessResourceDescriptorIndex(visibleMeshletCounterBuffer));
                    shaderConstants.BindBufferUAV(5, registry.GetBufferUAVBindlessResourceDescriptorIndex(drawIndirectArgumentBuffer));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VirtualGeometryMeshletCulling);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass("VisibilityBuffer", RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                drawIndirectArgumentBuffer = builder.ReadBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);
                visibleMeshletBuffer = builder.ReadBuffer(visibleMeshletBuffer, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle vbuffer0 = sceneTextures.vbuffer0 = builder.WriteTexture(sceneTextures.vbuffer0, RenderBackendResourceState::RenderTarget);
                RenderGraphTextureHandle vbuffer1 = sceneTextures.vbuffer1 = builder.WriteTexture(sceneTextures.vbuffer1, RenderBackendResourceState::RenderTarget);
                RenderGraphTextureHandle sceneDepthTexture = sceneTextures.sceneDepthTexture = builder.WriteTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);

                builder.BindRenderTarget(0, vbuffer0, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Store);
                builder.BindRenderTarget(1, vbuffer1, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Store);
                builder.BindDepthStencil(sceneDepthTexture,
                    RenderBackendRenderPassLoadOperation::Clear,
                    RenderBackendRenderPassStoreOperation::Store,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthWrite_StencilNoAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(renderResolution.width), float(renderResolution.height));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::VisibilityBufferVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::VisibilityBufferPS);

                    {
                        RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
                        graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
                        graphicsPipelineState.depthStencilState.depthTestEnable = true;
                        graphicsPipelineState.depthStencilState.depthWriteEnable = true;
                        graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
                        shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
                        shaderConstants.BindBufferSRV(3, registry.GetBufferSRVBindlessResourceDescriptorIndex(visibleMeshletBuffer));

                        commandList.DrawIndirect(
                            vertexShader,
                            pixelShader,
                            graphicsPipelineState,
                            shaderConstants,
                            registry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer),
                            0,
                            1,
                            RenderBackendPrimitiveTopology::TriangleList);
                    }
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

                builder.BindRenderTarget(0, vbuffer0, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Load);
                builder.BindRenderTarget(1, vbuffer1, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Load);
                builder.BindDepthStencil(sceneDepthTexture,
                    RenderBackendRenderPassLoadOperation::Clear,
                    RenderBackendRenderPassStoreOperation::Load,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthWrite_StencilNoAccess);

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
                        shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
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
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
                    shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0));
                    shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer1));
                    shaderConstants.BindTextureUAV(5, registry.GetTextureUAVBindlessResourceDescriptorIndex(gbuffer0, 0));
                    shaderConstants.BindTextureUAV(6, registry.GetTextureUAVBindlessResourceDescriptorIndex(gbuffer1, 0));
                    shaderConstants.BindTextureUAV(7, registry.GetTextureUAVBindlessResourceDescriptorIndex(gbuffer2, 0));

                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, 8);
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
        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();

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
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
                    shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0));
                    shaderConstants.BindTextureUAV(5, registry.GetTextureUAVBindlessResourceDescriptorIndex(motionVectorTexture, 0));

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
        RenderGraphTextureHandle localLightShadowMapAtlas)
    {
        RenderGraphTextureHandle skyAtmosphereTransmittanceLUT = defaultResources->ImportWhiteDummyTexture2D(renderGraph);
        if (IsSkyAtmosphereRenderingEnabled())
        {
            RealTimeRendererSkyAtmosphereLUTs& skyAtmosphereLUTs = renderGraph.blackboard.Get<RealTimeRendererSkyAtmosphereLUTs>();
            skyAtmosphereTransmittanceLUT = skyAtmosphereLUTs.transmittanceLut;
        }

        RenderBackendBufferHandle localLightDataBuffer = localLightDataBuffers[currentPerFrameDataBufferIndex];

        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();

        renderGraph.AddPass(
            std::format("DirectLighting (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                RealTimeRendererLightGridData& lightGridData = renderGraph.blackboard.Get<RealTimeRendererLightGridData>();

                RenderGraphTextureHandle vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle vbuffer1 = builder.ReadTexture(sceneTextures.vbuffer1, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer2 = builder.ReadTexture(sceneTextures.gbuffer2, RenderBackendResourceState::ShaderResource);
                // TODO: which state should be?
                //RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencilReadOnly);
                RenderGraphTextureHandle screenSpaceShadowMaskTexture = builder.ReadTexture(sceneTextures.shadowMaskTexture, RenderBackendResourceState::ShaderResource);
                localLightShadowMapAtlas = builder.ReadTexture(localLightShadowMapAtlas, RenderBackendResourceState::ShaderResource);
                skyAtmosphereTransmittanceLUT = builder.ReadTexture(skyAtmosphereTransmittanceLUT, RenderBackendResourceState::ShaderResource);

                RenderGraphBufferHandle lightGridCellDataBuffer = builder.ReadBuffer(lightGridData.lightGridCellDataBuffer, RenderBackendResourceState::ShaderResource);
                RenderGraphBufferHandle lightGridLightListBuffer = builder.ReadBuffer(lightGridData.lightGridLightListBuffer, RenderBackendResourceState::ShaderResource);

                RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindRenderTarget(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);
                builder.BindDepthStencil(sceneDepthTexture,
                    RenderBackendRenderPassLoadOperation::Load,
                    RenderBackendRenderPassStoreOperation::Store,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthReadOnly_StencilNoAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));  // TODO: fix crash when geometryBuffer is null
                    shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0));
                    shaderConstants.BindTextureSRV(5, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer1));
                    shaderConstants.BindTextureSRV(6, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                    shaderConstants.BindTextureSRV(7, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                    shaderConstants.BindTextureSRV(8, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer2));
                    shaderConstants.BindTextureSRV(9, registry.GetTextureSRVBindlessResourceDescriptorIndex(skyAtmosphereTransmittanceLUT));
                    shaderConstants.BindBufferSRV(10, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->lightDataBuffer));
                    shaderConstants.BindTextureSRV(11, registry.GetTextureSRVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture));
                    shaderConstants.BindTextureSRV(12, registry.GetTextureSRVBindlessResourceDescriptorIndex(localLightShadowMapAtlas));
                    shaderConstants.BindBufferSRV(13, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(view.scene->distantLightDataBuffer));
                    shaderConstants.BindBufferSRV(14, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(localLightDataBuffer));
                    shaderConstants.BindBufferSRV(15, registry.GetBufferSRVBindlessResourceDescriptorIndex(lightGridCellDataBuffer));
                    shaderConstants.BindBufferSRV(16, registry.GetBufferSRVBindlessResourceDescriptorIndex(lightGridLightListBuffer));

                    // TODO
                    shaderConstants.BindScalar(17, lightGridData.lightGridInfo.lightGridSizeX);
                    shaderConstants.BindScalar(18, lightGridData.lightGridInfo.lightGridSizeY);
                    shaderConstants.BindScalar(19, lightGridData.lightGridInfo.lightGridSizeZ);

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0] = RenderBackendColorBlendAttachmentState::Additive;

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::DrawFullscreenQuadVS);
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
        renderGraph.AddPass(
            std::format("IndirectDiffuseComposition (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                RenderGraphTextureHandle gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer2 = builder.ReadTexture(sceneTextures.gbuffer2, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencilReadOnly);
                RenderGraphBufferHandle irradianceEnvironmentMapBuffer = builder.ReadBuffer(sceneTextures.irradianceEnvironmentMapBuffer, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle ambientOcclusionTexture = builder.ReadTexture(sceneTextures.ambientOcclusionTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle indirectDiffuseTexture = builder.ReadTexture(sceneTextures.indirectDiffuseTexture, RenderBackendResourceState::ShaderResource);

                RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindRenderTarget(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);
                builder.BindDepthStencil(sceneDepthTexture,
                    RenderBackendRenderPassLoadOperation::Load,
                    RenderBackendRenderPassStoreOperation::Store,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthReadOnly_StencilNoAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
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

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer2));
                    shaderConstants.BindBufferSRV(4, registry.GetBufferSRVBindlessResourceDescriptorIndex(irradianceEnvironmentMapBuffer));
                    shaderConstants.BindBufferSRV(5, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(view.scene->irradianceEnvironmentMapTexture));
                    shaderConstants.BindTextureSRV(6, registry.GetTextureSRVBindlessResourceDescriptorIndex(ambientOcclusionTexture));
                    shaderConstants.BindTextureSRV(7, registry.GetTextureSRVBindlessResourceDescriptorIndex(indirectDiffuseTexture));

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::DrawFullscreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::IndirectDiffuseComposition);

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

    void RealTimeRenderer::AddIndirectLightingSpecularPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RenderBackendTextureHandle environmentBrdfLutTexture = defaultResources->GetEnvironmentBrdfLutTexture();

        renderGraph.AddPass(std::format("IndirectSpecularComposition (Graphics, {}x{})", renderResolution.width, renderResolution.height), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                RenderGraphTextureHandle gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer2 = builder.ReadTexture(sceneTextures.gbuffer2, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencilReadOnly);
                //RenderGraphTextureHandle screenSpaceReflectionTexture = builder.ReadTexture(sceneTextures.screenSpaceReflectionTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle ambientOcclusionTexture = builder.ReadTexture(sceneTextures.ambientOcclusionTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle convolvedEnvironmentMapTexture = builder.ReadTexture(sceneTextures.convolvedEnvironmentMapTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindRenderTarget(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);
                builder.BindDepthStencil(sceneDepthTexture,
                    RenderBackendRenderPassLoadOperation::Load,
                    RenderBackendRenderPassStoreOperation::Store,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthReadOnly_StencilNoAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0] = RenderBackendColorBlendAttachmentState::Additive;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer2));
                    shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    //shaderConstants.BindTextureSRV(5, registry.GetTextureSRVBindlessResourceDescriptorIndex(screenSpaceReflectionTexture));
                    shaderConstants.BindTextureSRV(6, registry.GetTextureSRVBindlessResourceDescriptorIndex(ambientOcclusionTexture));
                    shaderConstants.BindTextureSRV(7, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(environmentBrdfLutTexture));
                    shaderConstants.BindTextureSRV(8, registry.GetTextureSRVBindlessResourceDescriptorIndex(convolvedEnvironmentMapTexture));

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::DrawFullscreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::IndirectSpecularComposition);

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
}