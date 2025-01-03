#include "RealTimeRenderer.h"
#include "ShadowMapping.h"

namespace Horizon
{
    void RealTimeRenderer::RenderShadowMapDepth(
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
            RenderBackendBufferDesc cascadedShadowMapDataUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(sizeof(CascadedShadowMapShaderParameters));
            cascadedShadowMapDataUploadBuffer = renderBackend->CreateBuffer(&cascadedShadowMapDataUploadBufferDesc, nullptr, "CascadedShadowMapDataUploadBuffer");
            RenderBackendBufferDesc cascadedShadowMapDataBufferDesc = RenderBackendBufferDesc::CreateStructured(sizeof(CascadedShadowMapShaderParameters), 1);
            cascadedShadowMapDataBuffer = renderBackend->CreateBuffer(&cascadedShadowMapDataBufferDesc, nullptr, "CascadedShadowMapDataBuffer");
        }
        renderBackend->UpdateBuffer(cascadedShadowMapDataUploadBuffer, 0, &cascadedShadowMapShaderParameters, sizeof(CascadedShadowMapShaderParameters));

        renderGraph.AddPass(
            std::format("UpdateCascadedShadowMapDataBuffer (Copy, {} bytes)", sizeof(CascadedShadowMapShaderParameters)),
            RenderGraphPassFlags::Copy,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
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

        RenderGraphTextureDesc cascadedShadowMapDepthTextureDesc = RenderGraphTextureDesc::Create2DArray(
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

                builder.BindDepthStencil(cascadedShadowMapDepthTexture,
                    RenderBackendRenderPassBeginningAccessType::Clear,
                    RenderBackendRenderPassEndingAccessType::Preserve,
                    RenderBackendRenderPassBeginningAccessType::NoAccess,
                    RenderBackendRenderPassEndingAccessType::NoAccess,
                    RenderBackendDepthStencilAccessType::DepthWrite_StencilNoAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
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

        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        sceneTextures.cascadedShadowMapShaderParameterBuffer = cascadedShadowMapDataBuffer;
        sceneTextures.cascadedShadowMapDepthTexture = cascadedShadowMapDepthTexture;
    }

    void RealTimeRenderer::DispatchShadowMapProjection(RenderGraph& renderGraph, const SceneView& view)
    {
        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        RenderBackendBufferHandle& cascadedShadowMapShaderParameterBuffer = cascadedShadowMapShaderParameterBuffers[currentPerFrameDataBufferIndex];

        RenderGraphTextureDesc screenSpaceShadowMaskTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R8G8B8A8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture");

        // When a resource has the D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET flag, DiscardResource must be called when the discarded subresource regions are in the D3D12_RESOURCE_STATE_RENDER_TARGET resource barrier state.
        renderGraph.AddPass(
           std::format("DummyPass (Compute, {}x{})", screenSpaceShadowMaskTextureDesc.width, screenSpaceShadowMaskTextureDesc.height),
           RenderGraphPassFlags::Compute,
           [&](RenderGraphBuilder& builder)
           {
                screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::RenderTarget);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {

                };
           });

        renderGraph.AddPass(
            std::format("CascadedShadowMapProjection (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle cascadedShadowMapDepthTexture = builder.ReadTexture(sceneTextures.cascadedShadowMapDepthTexture, RenderBackendResourceState::ShaderResource);
                screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(cascadedShadowMapShaderParameterBuffer));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(cascadedShadowMapDepthTexture));
                    shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::ShadowMapProjectionForDistantLight);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        sceneTextures.shadowMaskTexture = screenSpaceShadowMaskTexture;
    }
}