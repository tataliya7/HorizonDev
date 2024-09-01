#include "RealTimeRenderer.h"
#include "Engine/Classes/Scene.h"

namespace Horizon
{
    // 8x8 piexels per tile
    static const uint32 VolumetricFogTileSize = 16;
    //
    static const uint32 VolumetricFogDepthSliceCount = 64;

    struct VolumetricFogShaderParameters
    {
        Vector3u volumeResolution;
        Vector4f tileCountAndInverseTileCount;
        Vector2f sliceCountAndInverseSliceCount;
    };

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

    void RealTimeRenderer::RenderVolumetricFog(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const uint32 volumetricFogTileSize = VolumetricFogTileSize;
        const uint32 volumetricFogTileCountX = CeilDiv(renderResolution.width, volumetricFogTileSize);
        const uint32 volumetricFogTileCountY = CeilDiv(renderResolution.height, volumetricFogTileSize);
        const uint32 volumetricFogDepthSliceCount = VolumetricFogDepthSliceCount;


        uint32 volumetricFogShaderParameterBufferSize = sizeof(VolumetricFogShaderParameters);

        VolumetricFogShaderParameters volumetricFogShaderParameters = {};
        volumetricFogShaderParameters.volumeResolution = Vector3u(volumetricFogTileCountX, volumetricFogTileCountY, volumetricFogDepthSliceCount);
        volumetricFogShaderParameters.tileCountAndInverseTileCount = Vector4f(float(volumetricFogTileCountX), float(volumetricFogTileCountY), 1.0f / float(volumetricFogTileCountX), 1.0f / float(volumetricFogTileCountY));
        volumetricFogShaderParameters.sliceCountAndInverseSliceCount = Vector2f(float(volumetricFogDepthSliceCount), 1.0f / float(volumetricFogDepthSliceCount));

        static RenderBackendBufferHandle volumetricFogShaderParameterBufferUpload;
        if (!volumetricFogShaderParameterBufferUpload)
        {
            RenderBackendBufferDesc volumetricFogShaderParameterBufferUploadDesc = RenderBackendBufferDesc::CreateUpload(volumetricFogShaderParameterBufferSize);
            volumetricFogShaderParameterBufferUpload = renderBackend->CreateBuffer(&volumetricFogShaderParameterBufferUploadDesc, nullptr, "VolumetricFogShaderParameterBufferUpload");
        }
        renderBackend->UpdateBuffer(volumetricFogShaderParameterBufferUpload, 0, &volumetricFogShaderParameters, volumetricFogShaderParameterBufferSize);

        RenderGraphBufferDesc volumetricFogShaderParameterBufferDesc = RenderGraphBufferDesc::CreateStructured(volumetricFogShaderParameterBufferSize, 1);
        RenderGraphBufferHandle volumetricFogShaderParameterBuffer = renderGraph.CreateBuffer(volumetricFogShaderParameterBufferDesc, "VolumetricFogShaderParameterBuffer");

        renderGraph.AddPass(
            std::format("UpdateVolumetricFog"),
            RenderGraphPassFlags::Copy,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.CopyBuffer(
                        volumetricFogShaderParameterBufferUpload,
                        0,
                        registry.GetRenderBackendBufferHandle(volumetricFogShaderParameterBuffer),
                        0,
                        volumetricFogShaderParameterBufferSize);
                };
            });

        RenderGraphTextureDesc volumetricFogCommonTextureDesc = RenderGraphTextureDesc::Create3D(
            volumetricFogTileCountX,
            volumetricFogTileCountY,
            volumetricFogDepthSliceCount,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);

        RenderGraphTextureHandle volumetricFogParticipatingMediaPropertyTextureA = renderGraph.CreateTexture(volumetricFogCommonTextureDesc, "VolumetricFogParticipatingMediaPropertyTextureA");
        RenderGraphTextureHandle volumetricFogParticipatingMediaPropertyTextureB = renderGraph.CreateTexture(volumetricFogCommonTextureDesc, "VolumetricFogParticipatingMediaPropertyTextureB");

        renderGraph.AddPass(
             std::format("VolumetricFogVoxelization (Compute, {}x{}x{})", volumetricFogTileCountX, volumetricFogTileCountY, volumetricFogDepthSliceCount),
             RenderGraphPassFlags::Compute,
             [&](RenderGraphBuilder& builder)
             {
                 volumetricFogParticipatingMediaPropertyTextureA = builder.WriteTexture(volumetricFogParticipatingMediaPropertyTextureA, RenderBackendResourceState::UnorderedAccess);
                 volumetricFogParticipatingMediaPropertyTextureB = builder.WriteTexture(volumetricFogParticipatingMediaPropertyTextureB, RenderBackendResourceState::UnorderedAccess);

                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                 {
                     uint32 threadGroupCountX = CeilDiv(volumetricFogTileCountX, 4);
                     uint32 threadGroupCountY = CeilDiv(volumetricFogTileCountY, 4);
                     uint32 threadGroupCountZ = CeilDiv(volumetricFogDepthSliceCount, 4);

                     RenderBackendShaderConstants shaderConstants = {};
                     shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                     shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(volumetricFogShaderParameterBuffer));
                     shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertyTextureA, 0));
                     shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertyTextureB, 0));

                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VolumetricFogVoxelization);

                     commandList.Dispatch(
                         computeShader,
                         shaderConstants,
                         threadGroupCountX,
                         threadGroupCountY,
                         threadGroupCountZ);
                 };
             });

        RenderGraphTextureHandle volumetricFogScatteringAndExtinctionTexture = renderGraph.CreateTexture(volumetricFogCommonTextureDesc, "VolumetricFogScatteringAndExtinctionTexture");

        renderGraph.AddPass(
            std::format("VolumetricFogLightScattering (Compute, {}x{}x{})", volumetricFogTileCountX, volumetricFogTileCountY, volumetricFogDepthSliceCount),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                volumetricFogParticipatingMediaPropertyTextureA = builder.ReadTexture(volumetricFogParticipatingMediaPropertyTextureA, RenderBackendResourceState::ShaderResource);
                volumetricFogParticipatingMediaPropertyTextureB = builder.ReadTexture(volumetricFogParticipatingMediaPropertyTextureB, RenderBackendResourceState::ShaderResource);
                //volumetricFogScatteringAndExtinctionTexture = builder.ReadTexture(volumetricFogScatteringAndExtinctionTexture, RenderBackendResourceState::ShaderResource);
                volumetricFogScatteringAndExtinctionTexture = builder.WriteTexture(volumetricFogScatteringAndExtinctionTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = CeilDiv(volumetricFogTileCountX, 4);
                    uint32 threadGroupCountY = CeilDiv(volumetricFogTileCountY, 4);
                    uint32 threadGroupCountZ = CeilDiv(volumetricFogDepthSliceCount, 4);

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(volumetricFogShaderParameterBuffer));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertyTextureA));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertyTextureA));
                    shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertyTextureB));
                    shaderConstants.BindTextureUAV(5, registry.GetTextureUAVBindlessResourceDescriptorIndex(volumetricFogScatteringAndExtinctionTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VolumetricFogLightScattering);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        RenderGraphTextureHandle volumetricFogIntegratedLightScatteringTexture = renderGraph.CreateTexture(volumetricFogCommonTextureDesc, "VolumetricFogIntegratedLightScatteringTexture");

        renderGraph.AddPass(
            std::format("VolumetricFogFinalIntegration (Compute, {}x{}x1)", volumetricFogTileCountX, volumetricFogTileCountY),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                volumetricFogScatteringAndExtinctionTexture = builder.ReadTexture(volumetricFogScatteringAndExtinctionTexture, RenderBackendResourceState::ShaderResource);
                volumetricFogIntegratedLightScatteringTexture = builder.WriteTexture(volumetricFogIntegratedLightScatteringTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = CeilDiv(volumetricFogTileCountX, 8);
                    uint32 threadGroupCountY = CeilDiv(volumetricFogTileCountY, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(volumetricFogShaderParameterBuffer));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(volumetricFogScatteringAndExtinctionTexture));
                    shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndex(volumetricFogIntegratedLightScatteringTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VolumetricFogFinalIntegration);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });
    }

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

        if (!localVolumetricFogInstanceDataBufferUpload)
        {
            RenderBackendBufferDesc localVolumetricFogInstanceDataBufferDesc = RenderBackendBufferDesc::CreateUpload(sizeof(LocalVolumetricFogInstanceData) * localVolumetricFogInstanceCount);
            //localVolumetricFogInstanceDataBufferDesc.flags |= RenderBackendBufferCreateFlags::CpuToGpu; // TODO
            localVolumetricFogInstanceDataBufferUpload = renderBackend->CreateBuffer(&localVolumetricFogInstanceDataBufferDesc, nullptr, "LocalVolumetricFogInstanceDataBuffer");
            localVolumetricFogInstanceDataBufferSize = sizeof(LocalVolumetricFogInstanceData) * localVolumetricFogInstanceCount;
        }
        else if (localVolumetricFogInstanceDataBufferSize < sizeof(LocalVolumetricFogInstanceData) * localVolumetricFogInstanceCount)
        {
            renderBackend->ResizeBuffer(localVolumetricFogInstanceDataBufferUpload, sizeof(LocalVolumetricFogInstanceData) * localVolumetricFogInstanceCount);
            localVolumetricFogInstanceDataBufferSize = sizeof(LocalVolumetricFogInstanceData) * localVolumetricFogInstanceCount;
        }

        void* data = nullptr;
        renderBackend->MapBuffer(localVolumetricFogInstanceDataBufferUpload, &data);
        memcpy(data, renderData.instanceData.data(), localVolumetricFogInstanceDataBufferSize);
        renderBackend->UnmapBuffer(localVolumetricFogInstanceDataBufferUpload);

        RenderGraphBufferDesc localVolumetricFogInstanceDataBufferDesc = RenderGraphBufferDesc::CreateStructured(sizeof(LocalVolumetricFogInstanceData), localVolumetricFogInstanceCount);
        RenderGraphBufferHandle localVolumetricFogInstanceDataBuffer = renderGraph.CreateBuffer(localVolumetricFogInstanceDataBufferDesc, "LocalVolumetricFogInstanceDataBuffer");

        renderGraph.AddPass(
            std::format("UpdateLocalVolumetricFogData"),
            RenderGraphPassFlags::Copy,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.CopyBuffer(
                        localVolumetricFogInstanceDataBufferUpload,
                        0,
                        registry.GetRenderBackendBufferHandle(localVolumetricFogInstanceDataBuffer),
                        0,
                        localVolumetricFogInstanceDataBufferSize);
                };
            });

        renderGraph.AddPass(
            std::format("LocalVolumetricFog (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencilReadOnly);
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
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(localVolumetricFogInstanceDataBuffer));
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