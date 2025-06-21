#include "../RasterizationRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    RenderGraphTextureHandle RasterizationRenderer::AddVisualizeDepthPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RasterizationRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RasterizationRendererSceneTextures>();

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

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::VisualizeDepth);

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

    RenderGraphTextureHandle RasterizationRenderer::AddVisualizeWorldSpaceNormalPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RasterizationRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RasterizationRendererSceneTextures>();

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

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                    shaderConstants.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::VisualizeWorldSpaceNormal);

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

    RenderGraphTextureHandle RasterizationRenderer::AddVisualizeMotionVectorsPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RasterizationRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RasterizationRendererSceneTextures>();

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

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(motionVectorTexture));
                    shaderConstants.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::VisualizeMotionVectors);

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

    RenderGraphTextureHandle RasterizationRenderer::AddVisualizeAmbientOcclusionPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RasterizationRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RasterizationRendererSceneTextures>();

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

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(ambientOcclusionTexture));
                    shaderConstants.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::VisualizeAmbientOcclusion);

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

    RenderGraphTextureHandle RasterizationRenderer::AddVisualizeShadowMaskPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture)
    {
        return RenderGraphTextureHandle::Null;
        // RasterizationRendererDebugViewModeTextures& debugViewModeTextures = renderGraph.blackboard.Get<RasterizationRendererDebugViewModeTextures>();
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
        //
        //         {
        //             uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
        //             uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
        //             uint32 threadGroupCountZ = 1;
        //
        //             RenderBackendPushConstantValues shaderConstants = {};
        //             shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //             shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture));
        //             shaderConstants.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));
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

    RenderGraphTextureHandle RasterizationRenderer::AddVisualizeCascadedShadowMapPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RasterizationRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RasterizationRendererSceneTextures>();

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

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(debugVisualizationTexture));
                    shaderConstants.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::VisualizeCascadedShadowMap);

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
                sceneColorTexture = builder.WriteTexture(sceneColorTexture, RenderBackendResourceState::RenderTarget);
                sceneDepthTexture = builder.WriteTexture(sceneDepthTexture, RenderBackendResourceState::DepthStencil);

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

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(debugDrawLinesVertexBuffer));

                    RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::DebugDrawVS);
                    RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::DebugDrawPS);

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