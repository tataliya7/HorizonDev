#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeDepthPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");
        //RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(view.targetTexture->GetDesc(), "VisualizeDepthTexture");

        renderGraph.AddPass(
            std::format("VisualizePrimitiveID (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizeDepth);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddVisualizePrimitiveIDPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");
        //RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(view.targetTexture->GetDesc(), "VisualizePrimitiveIDTexture");

        renderGraph.AddPass(
            std::format("VisualizePrimitiveID (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0));
                    shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizePrimitiveID);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeMaterialIDPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");
        //RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(view.targetTexture->GetDesc(), "VisualizeMaterialIDTexture");

        renderGraph.AddPass(
            std::format("VisualizeMaterialID (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(view.scene->gpuScene->geometryDataBuffer));
                    shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(view.scene->gpuScene->geometryInstanceDataBuffer));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(vbuffer0));
                    shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizeMaterialID);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeWorldSpaceNormalPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");
        //RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(view.targetTexture->GetDesc(), "VisualizeWorldSpaceNormalTexture");

        renderGraph.AddPass(
            std::format("VisualizeWorldSpaceNormal (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                    shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizeWorldSpaceNormal);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeMotionVectorsPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");
        //RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(view.targetTexture->GetDesc(), "VisualizeMotionVectorsTexture");

        renderGraph.AddPass(
            std::format("VisualizeMotionVectors (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle motionVectorTexture = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(motionVectorTexture));
                    shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizeMotionVectors);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeAmbientOcclusionPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");
        //RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(view.targetTexture->GetDesc(), "VisualizeAmbientOcclusionTexture");

        renderGraph.AddPass(
            std::format("VisualizeAmbientOcclusion (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle ambientOcclusionTexture = builder.ReadTexture(sceneTextures.ambientOcclusionTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(ambientOcclusionTexture));
                    shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizeAmbientOcclusion);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeShadowMaskPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture)
    {
        return RenderGraphTextureHandle::Null;
        // RealTimeRendererDebugViewModeTextures& debugViewModeTextures = renderGraph.blackboard.Get<RealTimeRendererDebugViewModeTextures>();
        //
        // if (!debugViewModeTextures.screenSpaceShadowMaskTexture)
        // {
        //     return sceneColorTexture;
        // }
        //
        // // TODO
        // RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");
        // //RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(view.targetTexture->GetDesc(), "VisualizeScreenSpaceShadowMaskTexture");
        //
        // uint32 width = debugViewModeTextures.screenSpaceShadowMaskTextureDesc.width;
        // uint32 height = debugViewModeTextures.screenSpaceShadowMaskTextureDesc.height;
        //
        // renderGraph.AddPass(
        //     std::format("VisualizeScreenSpaceShadowMask (Compute, {}x{})", width, height, targetResolution.width, targetResolution.height),
        //     RenderGraphPassFlags::Compute,
        //     [&](RenderGraphBuilder& builder)
        //     {
        //         auto screenSpaceShadowMaskTexture = builder.ReadTexture(debugViewModeTextures.screenSpaceShadowMaskTexture, RenderBackendResourceState::ShaderResource);
        //
        //         outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);
        //
        //         return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //         {
        //             uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
        //             uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
        //             uint32 threadGroupCountZ = 1;
        //
        //             RenderBackendShaderConstants shaderConstants = {};
        //             shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //             shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture));
        //             shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));
        //
        //             RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizeScreenSpaceShadowMask);
        //
        //             commandList.Dispatch(
        //                 computeShader,
        //                 shaderConstants,
        //                 threadGroupCountX,
        //                 threadGroupCountY,
        //                 threadGroupCountZ);
        //         };
        //     });
        //
        // return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeCascadedShadowMapPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");

        renderGraph.AddPass(
            std::format("VisualizeCascadedShadowMap (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle debugVisualizationTexture = builder.ReadTexture(sceneTextures.cascadedShadowMapDebugVisualizationTexture, RenderBackendResourceState::ShaderResource);

                debugVisualizationTexture = builder.ReadTexture(debugVisualizationTexture, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(debugVisualizationTexture));
                    shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizeCascadedShadowMap);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeVirtualShadowMapPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");

        renderGraph.AddPass(
            std::format("VisualizeVirtualShadowMap (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle debugVisualizationTexture = builder.ReadTexture(sceneTextures.virtualShadowMapDebugVisualizationTexture, RenderBackendResourceState::ShaderResource);

                debugVisualizationTexture = builder.ReadTexture(debugVisualizationTexture, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(debugVisualizationTexture));
                    shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizeVirtualShadowMap);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddDebugDrawPass(
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
            RenderBackendBufferDesc bufferDesc = RenderBackendBufferDesc::CreateByteAddress(newDebugDrawLinesVertexBufferSize);
            debugDrawLinesVertexBuffer = renderBackend->CreateBuffer(&bufferDesc, nullptr, "DebugDrawLinesVertexBuffer");

            RenderBackendBufferDesc debugDrawLinesVertexUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(newDebugDrawLinesVertexBufferSize);
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

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
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
                sceneColorTexture = builder.WriteTexture(sceneColorTexture, RenderBackendResourceState::RenderTarget);
                sceneDepthTexture = builder.WriteTexture(sceneDepthTexture, RenderBackendResourceState::DepthStencil);

                builder.BindRenderTarget(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);
                builder.BindDepthStencil(
                    sceneDepthTexture,
                    RenderBackendRenderPassLoadOperation::Load,
                    RenderBackendRenderPassStoreOperation::Store,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthWrite_StencilNoAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    // TODO: MSAA
                    // TODO: Remove pixel shader
                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.rasterizationState.lineWidth = 2.0f;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(debugDrawLinesVertexBuffer));

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::DebugDrawVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::DebugDrawPS);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderConstants,
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