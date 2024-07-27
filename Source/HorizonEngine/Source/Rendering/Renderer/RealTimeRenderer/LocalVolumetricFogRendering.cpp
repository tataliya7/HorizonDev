#include "RealTimeRenderer.h"
#include "Engine/Classes/Scene.h"

namespace Horizon
{
    struct LocalVolumetricFogInstanceData
    {
        Matrix4x4f localToWorldMatrix;
        Vector3f emission;
    };

    struct LocalVolumetricFogRenderData
    {
        uint32 instanceCount;
        std::vector<LocalVolumetricFogInstanceData> instanceData;
    };

    void RealTimeRenderer::RenderLocalVolumetricFogs(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        uint32 localVolumetricFogInstanceCount = uint32(view.scene->localVolumetricFogs.size());

        if (localVolumetricFogInstanceCount == 0)
        {
            return;
        }

        LocalVolumetricFogRenderData renderData = {};
        renderData.instanceCount = localVolumetricFogInstanceCount;
        renderData.instanceData.resize(localVolumetricFogInstanceCount);

        for (uint32 index = 0; index < localVolumetricFogInstanceCount; index++)
        {
            LocalVolumetricFogRenderObject* localVolumetricFog = view.scene->localVolumetricFogs[index];

            LocalVolumetricFogInstanceData& instanceData = renderData.instanceData[index];
            instanceData.localToWorldMatrix = localVolumetricFog->transform;
            instanceData.emission = localVolumetricFog->emission;
        }

        if (!localVolumetricFogInstanceDataBuffer)
        {
            RenderBackendBufferDesc localVolumetricFogInstanceDataBufferDesc = RenderBackendBufferDesc::CreateStructured(sizeof(LocalVolumetricFogInstanceData), localVolumetricFogInstanceCount);
            localVolumetricFogInstanceDataBuffer = renderBackend->CreateBuffer(&localVolumetricFogInstanceDataBufferDesc, nullptr, "LocalVolumetricFogInstanceDataBuffer");
            localVolumetricFogInstanceDataBufferSize = sizeof(LocalVolumetricFogInstanceData) * localVolumetricFogInstanceCount;
        }
        else if (localVolumetricFogInstanceDataBufferSize < sizeof(LocalVolumetricFogInstanceData) * localVolumetricFogInstanceCount)
        {
            renderBackend->ResizeBuffer(localVolumetricFogInstanceDataBuffer, sizeof(LocalVolumetricFogInstanceData) * localVolumetricFogInstanceCount);
            localVolumetricFogInstanceDataBufferSize = sizeof(LocalVolumetricFogInstanceData) * localVolumetricFogInstanceCount;
        }

        void* data = nullptr;
        renderBackend->MapBuffer(localVolumetricFogInstanceDataBuffer, &data);
        memcpy(data, renderData.instanceData.data(), localVolumetricFogInstanceDataBufferSize);
        renderBackend->UnmapBuffer(localVolumetricFogInstanceDataBuffer);

        renderGraph.AddPass(
            std::format("LocalVolumetricFog (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);
                RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindRenderTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);
                builder.BindDepthStencil(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::SrcAlpha;
                    graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcAlphaBlendFactor = RenderBackendBlendFactor::Zero;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstAlphaBlendFactor = RenderBackendBlendFactor::SrcAlpha;
                    graphicsPipelineState.colorBlendState.targetBlends[0].alphaBlendOp = RenderBackendBlendOp::Add;
                    graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::RGBA;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(localVolumetricFogInstanceDataBuffer));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::LocalVolumetricFogVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::LocalVolumetricFogPS);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderConstants,
                        36, localVolumetricFogInstanceCount, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }
}