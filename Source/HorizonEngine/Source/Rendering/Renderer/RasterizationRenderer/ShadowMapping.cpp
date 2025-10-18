#include "RasterizationRenderer.h"
#include "ShadowMapping.h"

namespace Horizon
{
    void RasterizationRenderer::RenderShadowMapDepth(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const LightRenderObject* lightPtr = nullptr;
        for (uint32 lightIndex = 0; lightIndex < uint32(view.scene->lights.size()); lightIndex++)
        {
            lightPtr = view.scene->lights[lightIndex];
            if (lightPtr->lightType == LightType::DistantLight)
            {
                break;
            }
        }
        const LightRenderObject& light = *lightPtr;

        CascadedShadowMapShaderParameters cascadedShadowMapShaderParameters;
        SetupCascadedShadowMapShaderParameters(cascadedShadowMapShaderParameters, view, light, cascadedShadowMapRenderData);

        RenderBackendBufferHandle& cascadedShadowMapDataUploadBuffer = cascadedShadowMapShaderParameterUploadBuffers[currentPerFrameDataBufferIndex];
        RenderBackendBufferHandle& cascadedShadowMapDataBuffer = cascadedShadowMapShaderParameterBuffers[currentPerFrameDataBufferIndex];
        if (!cascadedShadowMapDataBuffer)
        {
            RenderBackendBufferDescription cascadedShadowMapDataUploadBufferDesc = RenderBackendBufferDescription::CreateUpload(sizeof(CascadedShadowMapShaderParameters));
            cascadedShadowMapDataUploadBuffer = renderBackend->CreateBuffer(&cascadedShadowMapDataUploadBufferDesc, nullptr, "CascadedShadowMapDataUploadBuffer");
            RenderBackendBufferDescription cascadedShadowMapDataBufferDesc = RenderBackendBufferDescription::CreateStructured(sizeof(CascadedShadowMapShaderParameters), 1);
            cascadedShadowMapDataBuffer = renderBackend->CreateBuffer(&cascadedShadowMapDataBufferDesc, nullptr, "CascadedShadowMapDataBuffer");
        }
        renderBackend->UpdateBuffer(cascadedShadowMapDataUploadBuffer, 0, &cascadedShadowMapShaderParameters, sizeof(CascadedShadowMapShaderParameters));

        renderGraph.AddPass(
            std::format("UpdateCascadedShadowMapDataBuffer (Copy, {} bytes)", sizeof(CascadedShadowMapShaderParameters)),
            RenderGraphPassFlags::Copy,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.CopyBuffer(
                        cascadedShadowMapDataUploadBuffer,
                        0,
                        cascadedShadowMapDataBuffer,
                        0,
                        sizeof(CascadedShadowMapShaderParameters));
                };
            });

        const uint32 shadowCascadeCount = light.GetShadowCascadeCount();
        const uint32 shadowMapSize = light.GetShadowMapSize();

        RenderGraphTextureDescription cascadedShadowMapDepthTextureDesc = RenderGraphTextureDescription::Create2DArray(
            shadowMapSize,
            shadowMapSize,
            RenderBackendTextureFormat::D32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::DepthStencil,
            shadowCascadeCount,
            RenderBackendTextureClearValue::CreateDepthValue(FAR_CLIPPING_PLANE_DEPTH_VALUE));

        RenderGraphTextureHandle cascadedShadowMapDepthTexture = renderGraph.CreateTexture(cascadedShadowMapDepthTextureDesc, "CascadedShadowMapDepthTexture");

        renderGraph.AddPass(
            std::format("CascadedShadowMapDepth (Graphics, {}x{})", shadowMapSize, shadowMapSize),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                cascadedShadowMapDepthTexture = builder.WriteTexture(cascadedShadowMapDepthTexture, RenderBackendResourceState::DepthStencil);

                builder.SetDepthStencilBinding(cascadedShadowMapDepthTexture,
                    RenderBackendRenderPassLoadOperation::Clear,
                    RenderBackendRenderPassStoreOperation::Store,
                    RenderBackendRenderPassLoadOperation::None,
                    RenderBackendRenderPassStoreOperation::None,
                    RenderBackendDepthStencilAccessType::DepthWrite_StencilNoAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    {
                        RenderBackendViewport viewport(0.0f, 0.0f, float(shadowMapSize), float(shadowMapSize));
                        commandList.SetViewports(&viewport, 1);

                        RenderBackendScissor scissor(0, 0, shadowMapSize, shadowMapSize);
                        commandList.SetScissors(&scissor, 1);
                    }

                    for (uint32 cascadeIndex = 0; cascadeIndex < shadowCascadeCount; cascadeIndex++)
                    {
                        DispatchCascadedShadowMapPassDrawCommands(commandList, light, cascadeIndex, cascadedShadowMapDataBuffer);
                    }

                    {
                        RenderBackendViewport viewport(0.0f, 0.0f, float(renderResolution.width), float(renderResolution.height));
                        commandList.SetViewports(&viewport, 1);

                        RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                        commandList.SetScissors(&scissor, 1);
                    }
                };
            });

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();
        intermediateResources.cascadedShadowMapShaderParameterBuffer = cascadedShadowMapDataBuffer;
        intermediateResources.cascadedShadowMapDepthTexture = cascadedShadowMapDepthTexture;
    }

    void RasterizationRenderer::DispatchShadowMapProjection(RenderGraph& renderGraph, const SceneView& view)
    {
        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        RenderBackendBufferHandle& cascadedShadowMapShaderParameterBuffer = cascadedShadowMapShaderParameterBuffers[currentPerFrameDataBufferIndex];

        RenderGraphTextureDescription screenSpaceShadowMaskTextureDesc = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R8G8B8A8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture");

        RenderGraphTextureHandle cascadedShadowMapDebugVisualizationTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "CascadedShadowMapDebugVisualizationTexture");

        renderGraph.AddPass(
            std::format("CascadedShadowMapProjection (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

                RenderGraphTextureHandle depthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle normalTexture = builder.ReadTexture(intermediateResources.gbuffer0, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle cascadedShadowMapDepthTexture = builder.ReadTexture(intermediateResources.cascadedShadowMapDepthTexture, RenderBackendResourceState::ShaderResource);
                screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);
                cascadedShadowMapDebugVisualizationTexture = builder.WriteTexture(cascadedShadowMapDebugVisualizationTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(cascadedShadowMapShaderParameterBuffer));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(depthTexture));
                    pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(normalTexture));
                    pushConstantValues.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(cascadedShadowMapDepthTexture));
                    pushConstantValues.BindTextureUAV(5, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture, 0));
                    pushConstantValues.BindTextureUAV(6, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(cascadedShadowMapDebugVisualizationTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::ShadowMapProjectionForDistantLight);

                    //commandList.ClearTextureUAV(RenderBackendTextureUAVDesc(resourceRegistry.GetRenderBackendTextureHandle(screenSpaceShadowMaskTexture), 0), RenderBackendTextureClearValue::White);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        intermediateResources.shadowMaskTexture = screenSpaceShadowMaskTexture;
        intermediateResources.cascadedShadowMapDebugVisualizationTexture = cascadedShadowMapDebugVisualizationTexture;

        if (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::CascadedShadowMapCascadeIndex)
        {
            debugVisualizationCallback = std::bind(&RasterizationRenderer::DispatchCascadedShadowMapDebugVisualization, this, std::placeholders::_1, std::placeholders::_2);
        }
    }

    RenderGraphTextureHandle RasterizationRenderer::RenderLocalLightShadows(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        return defaultResources->ImportWhiteDummyTexture2D(renderGraph);

        static const uint32 cubeShadowMapSize = 256;
        static const uint32 localLightShadowMapAtlasSize = 4096;
        RenderGraphTextureDescription localLightShadowMapAtlasDesc = RenderGraphTextureDescription::Create2DArray(
            localLightShadowMapAtlasSize,
            localLightShadowMapAtlasSize,
            RenderBackendTextureFormat::D32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::DepthStencil,
            2,
            RenderBackendTextureClearValue::CreateDepthValue(FAR_CLIPPING_PLANE_DEPTH_VALUE));
        RenderGraphTextureHandle localLightShadowMapAtlas = renderGraph.CreateTexture(localLightShadowMapAtlasDesc, "LocalLightShadowMapAtlas");

        // uint32 numViewports = renderEngine->numCubeShadowMaps * 6;
        //
        // renderGraph.AddPass(std::format("LocalLightShadows (Graphics, {}x{}, {} viewports)", localLightShadowMapAtlasSize, localLightShadowMapAtlasSize, numViewports), RenderGraphPassFlags::Graphics,
        //     [&](RenderGraphBuilder& builder)
        //     {
        //         localLightShadowMapAtlas = builder.WriteTexture(localLightShadowMapAtlas, RenderBackendResourceState::DepthStencil);
        //
        //         builder.BindDepthTarget(localLightShadowMapAtlas, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Load);
        //
        //
        //         {
        //             RenderBackendViewport viewports[RenderBackendMaxViewportCount];
        //             RenderBackendScissor scissors[RenderBackendMaxViewportCount];
        //
        //             uint32 viewportIndex = 0;
        //             for (uint32 cubeShadowMapIndex = 0; cubeShadowMapIndex < renderEngine->numCubeShadowMaps; cubeShadowMapIndex++)
        //             {
        //                 for (uint32 faceIndex = 0; faceIndex < 6; faceIndex++)
        //                 {
        //                     viewports[viewportIndex] = RenderBackendViewport((float)faceIndex * cubeShadowMapSize, (float)cubeShadowMapIndex * cubeShadowMapSize, (float)cubeShadowMapSize, (float)cubeShadowMapSize);
        //                     scissors[viewportIndex] = RenderBackendScissor(0, 0, localLightShadowMapAtlasSize, localLightShadowMapAtlasSize);
        //                     viewportIndex++;
        //                 }
        //             }
        //
        //             commandList.SetViewports(viewports, numViewports);
        //             commandList.SetScissors(scissors, numViewports);
        //
        //             RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
        //             graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
        //             graphicsPipelineState.rasterizationState.depthBiasSlopeFactor = -1.5f;
        //             graphicsPipelineState.rasterizationState.depthBiasConstantFactor = -1.25f;
        //             graphicsPipelineState.depthStencilState.depthTestEnable = true;
        //             graphicsPipelineState.depthStencilState.depthWriteEnable = true;
        //             graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;
        //
        //             for (const auto& drawCallInfo : renderEngine->drawList)
        //             {
        //                 RenderBackendPushConstantValues pushConstantValues = {};
        //                 pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //                 pushConstantValues.BindBuffer(1, renderEngine->geometryBuffer, drawCallInfo.geometryIndex * sizeof(GeometryShaderParameters));
        //                 pushConstantValues.BindBuffer(2, renderEngine->materialBuffer, 0);
        //                 pushConstantValues.BindBuffer(3, renderEngine->cubeShadowMapBuffer, sizeof(CubeShadowMapShaderParameters));
        //
        //                 RenderBackendShaderHandle graphicsShader = shaderRepository->GetShader(ShaderID::LocalLightShadows);
        //
        //                 commandList.DrawIndexed(
        //                     graphicsShader,
        //                     graphicsPipelineState,
        //                     pushConstantValues,
        //                     drawCallInfo.indexBuffer,
        //                     drawCallInfo.indexCount,
        //                     numViewports,
        //                     drawCallInfo.firstIndex,
        //                     0,
        //                     0,
        //                     RenderBackendPrimitiveTopology::TriangleList);
        //             }
        //
        //             // Restore viewport and scissor
        //             RenderBackendViewport viewport(0.0f, 0.0f, (float)renderResolution.width, (float)renderResolution.height);
        //             commandList.SetViewports(&viewport, 1);
        //
        //             RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
        //             commandList.SetScissors(&scissor, 1);
        //         };
        //     });
        //
        // return localLightShadowMapAtlas;
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
        //             RenderBackendPushConstantValues pushConstantValues = {};
        //             pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //             pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture));
        //             pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));
        //
        //             RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VisualizeScreenSpaceShadowMask);
        //
        //             commandList.Dispatch(
        //                 computeShader,
        //                 pushConstantValues,
        //                 threadGroupCountX,
        //                 threadGroupCountY,
        //                 threadGroupCountZ);
        //         };
        //     });
        //
        // return outputTexture;
    }
}