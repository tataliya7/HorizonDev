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
        uint32 localFogVolumeInstanceCount;
    };

    struct LocalFogVolumeInstanceData
    {
        Matrix4x4f localToWorldMatrix;
        Vector3f scattering;
        Vector3f absorption;
        Vector3f emission;
    };

    struct LocalFogVolumeRenderData
    {
        uint32 instanceCount;
        std::vector<LocalFogVolumeInstanceData> instanceData;
    };

    void RealTimeRenderer::RenderVolumetricFog(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        return;
        uint32 localFogVolumeInstanceCount = uint32(view.scene->localFogVolumes.size());

        if (localFogVolumeInstanceCount == 0)
        {
            return;
        }

        LocalFogVolumeRenderData renderData = {};
        renderData.instanceCount = localFogVolumeInstanceCount;
        renderData.instanceData.resize(localFogVolumeInstanceCount);

        for (uint32 index = 0; index < localFogVolumeInstanceCount; index++)
        {
            LocalFogVolumeRenderObject* localFogVolume = view.scene->localFogVolumes[index];

            LocalFogVolumeInstanceData& instanceData = renderData.instanceData[index];
            instanceData.localToWorldMatrix = localFogVolume->transform;
            instanceData.scattering = localFogVolume->scattering;
            instanceData.absorption = localFogVolume->absorption;
            instanceData.emission = localFogVolume->emission;
        }

        if (!localFogVolumeInstanceDataBufferUpload)
        {
            RenderBackendBufferDesc localFogVolumeInstanceDataBufferDesc = RenderBackendBufferDesc::CreateUpload(sizeof(LocalFogVolumeInstanceData) * localFogVolumeInstanceCount);
            //localFogVolumeInstanceDataBufferDesc.flags |= RenderBackendBufferCreateFlags::CpuToGpu; // TODO
            localFogVolumeInstanceDataBufferUpload = renderBackend->CreateBuffer(&localFogVolumeInstanceDataBufferDesc, nullptr, "LocalFogVolumeInstanceDataBuffer");
            localFogVolumeInstanceDataBufferSize = sizeof(LocalFogVolumeInstanceData) * localFogVolumeInstanceCount;
        }
        else if (localFogVolumeInstanceDataBufferSize < sizeof(LocalFogVolumeInstanceData) * localFogVolumeInstanceCount)
        {
            renderBackend->ResizeBuffer(localFogVolumeInstanceDataBufferUpload, sizeof(LocalFogVolumeInstanceData) * localFogVolumeInstanceCount);
            localFogVolumeInstanceDataBufferSize = sizeof(LocalFogVolumeInstanceData) * localFogVolumeInstanceCount;
        }

        void* data = nullptr;
        renderBackend->MapBuffer(localFogVolumeInstanceDataBufferUpload, &data);
        memcpy(data, renderData.instanceData.data(), localFogVolumeInstanceDataBufferSize);
        renderBackend->UnmapBuffer(localFogVolumeInstanceDataBufferUpload);

        RenderGraphBufferDesc localFogVolumeInstanceDataBufferDesc = RenderGraphBufferDesc::CreateStructured(sizeof(LocalFogVolumeInstanceData), localFogVolumeInstanceCount);
        RenderGraphBufferHandle localFogVolumeInstanceDataBuffer = renderGraph.CreateBuffer(localFogVolumeInstanceDataBufferDesc, "LocalFogVolumeInstanceDataBuffer");

        renderGraph.AddPass(
            std::format("UpdateLocalFogVolumeData"),
            RenderGraphPassFlags::Copy,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.CopyBuffer(
                        localFogVolumeInstanceDataBufferUpload,
                        0,
                        registry.GetRenderBackendBufferHandle(localFogVolumeInstanceDataBuffer),
                        0,
                        localFogVolumeInstanceDataBufferSize);
                };
            });



        const uint32 volumetricFogTileSize = VolumetricFogTileSize;
        const uint32 volumetricFogTileCountX = CeilDiv(renderResolution.width, volumetricFogTileSize);
        const uint32 volumetricFogTileCountY = CeilDiv(renderResolution.height, volumetricFogTileSize);
        const uint32 volumetricFogDepthSliceCount = VolumetricFogDepthSliceCount;

        uint32 volumetricFogShaderParameterBufferSize = sizeof(VolumetricFogShaderParameters);

        VolumetricFogShaderParameters volumetricFogShaderParameters = {};
        volumetricFogShaderParameters.volumeResolution = Vector3u(volumetricFogTileCountX, volumetricFogTileCountY, volumetricFogDepthSliceCount);
        volumetricFogShaderParameters.tileCountAndInverseTileCount = Vector4f(float(volumetricFogTileCountX), float(volumetricFogTileCountY), 1.0f / float(volumetricFogTileCountX), 1.0f / float(volumetricFogTileCountY));
        volumetricFogShaderParameters.sliceCountAndInverseSliceCount = Vector2f(float(volumetricFogDepthSliceCount), 1.0f / float(volumetricFogDepthSliceCount));
        volumetricFogShaderParameters.localFogVolumeInstanceCount = localFogVolumeInstanceCount;

        static RenderBackendBufferHandle volumetricFogShaderParameterBufferUpload;
        if (!volumetricFogShaderParameterBufferUpload)
        {
            RenderBackendBufferDesc volumetricFogShaderParameterBufferUploadDesc = RenderBackendBufferDesc::CreateUpload(volumetricFogShaderParameterBufferSize);
            volumetricFogShaderParameterBufferUpload = renderBackend->CreateBuffer(&volumetricFogShaderParameterBufferUploadDesc, nullptr, "VolumetricFogShaderParameterBufferUpload");
        }
        renderBackend->UpdateBuffer(volumetricFogShaderParameterBufferUpload, 0, &volumetricFogShaderParameters, volumetricFogShaderParameterBufferSize);

        RenderGraphBufferDesc volumetricFogShaderParameterBufferDesc = RenderGraphBufferDesc::CreateStructured(volumetricFogShaderParameterBufferSize, 1);
        RenderGraphBufferHandle volumetricFogShaderParameterBuffer = renderGraph.CreateBuffer(volumetricFogShaderParameterBufferDesc, "VolumetricFogShaderParameterBuffer");

        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

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

        RenderGraphTextureHandle volumetricFogParticipatingMediaPropertiesDataATexture = renderGraph.CreateTexture(volumetricFogCommonTextureDesc, "VolumetricFogParticipatingPropertiesDataATexture");
        RenderGraphTextureHandle volumetricFogParticipatingMediaPropertiesDataBTexture = renderGraph.CreateTexture(volumetricFogCommonTextureDesc, "VolumetricFogParticipatingPropertiesDataBTexture");

        renderGraph.AddPass(
             std::format("VolumetricFogVoxelization (Compute, {}x{}x{})", volumetricFogTileCountX, volumetricFogTileCountY, volumetricFogDepthSliceCount),
             RenderGraphPassFlags::Compute,
             [&](RenderGraphBuilder& builder)
             {
                 volumetricFogParticipatingMediaPropertiesDataATexture = builder.WriteTexture(volumetricFogParticipatingMediaPropertiesDataATexture, RenderBackendResourceState::UnorderedAccess);
                 volumetricFogParticipatingMediaPropertiesDataBTexture = builder.WriteTexture(volumetricFogParticipatingMediaPropertiesDataBTexture, RenderBackendResourceState::UnorderedAccess);

                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                 {
                     uint32 threadGroupCountX = CeilDiv(volumetricFogTileCountX, 4);
                     uint32 threadGroupCountY = CeilDiv(volumetricFogTileCountY, 4);
                     uint32 threadGroupCountZ = CeilDiv(volumetricFogDepthSliceCount, 4);

                     RenderBackendShaderConstants shaderConstants = {};
                     shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                     shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(volumetricFogShaderParameterBuffer));
                     shaderConstants.BindBufferSRV(2, registry.GetBufferSRVBindlessResourceDescriptorIndex(localFogVolumeInstanceDataBuffer));
                     shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertiesDataATexture, 0));
                     shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertiesDataBTexture, 0));

                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VolumetricFogVoxelization);

                     commandList.Dispatch(
                         computeShader,
                         shaderConstants,
                         threadGroupCountX,
                         threadGroupCountY,
                         threadGroupCountZ);
                 };
             });

        RenderGraphTextureHandle previousVolumetricFogLightScatteringTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        if (historyFrame.volumetricFogLightScatteringTexture != nullptr)
        {
            previousVolumetricFogLightScatteringTexture = renderGraph.ImportExternalTexture(historyFrame.volumetricFogLightScatteringTexture, "PreviousVolumetricFogLightScatteringTexture");
        }

        RenderGraphTextureHandle volumetricFogLightScatteringTexture = renderGraph.CreateTexture(volumetricFogCommonTextureDesc, "VolumetricFogLightScatteringTexture");

        renderGraph.AddPass(
            std::format("VolumetricFogLightScattering (Compute, {}x{}x{})", volumetricFogTileCountX, volumetricFogTileCountY, volumetricFogDepthSliceCount),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle shadowMapTexture = builder.ReadTexture(sceneTextures.cascadedShadowMapDepthTexture, RenderBackendResourceState::ShaderResource);
                volumetricFogParticipatingMediaPropertiesDataATexture = builder.ReadTexture(volumetricFogParticipatingMediaPropertiesDataATexture, RenderBackendResourceState::ShaderResource);
                volumetricFogParticipatingMediaPropertiesDataBTexture = builder.ReadTexture(volumetricFogParticipatingMediaPropertiesDataBTexture, RenderBackendResourceState::ShaderResource);
                previousVolumetricFogLightScatteringTexture = builder.ReadTexture(previousVolumetricFogLightScatteringTexture, RenderBackendResourceState::ShaderResource);
                volumetricFogLightScatteringTexture = builder.WriteTexture(volumetricFogLightScatteringTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = CeilDiv(volumetricFogTileCountX, 4);
                    uint32 threadGroupCountY = CeilDiv(volumetricFogTileCountY, 4);
                    uint32 threadGroupCountZ = CeilDiv(volumetricFogDepthSliceCount, 4);

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(volumetricFogShaderParameterBuffer));
                    shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(view.scene->distantLightDataBuffer));
                    shaderConstants.BindBufferSRV(3, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(sceneTextures.cascadedShadowMapShaderParameterBuffer));
                    shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(shadowMapTexture));
                    shaderConstants.BindTextureSRV(5, registry.GetTextureSRVBindlessResourceDescriptorIndex(previousVolumetricFogLightScatteringTexture));
                    shaderConstants.BindTextureSRV(6, registry.GetTextureSRVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertiesDataATexture));
                    shaderConstants.BindTextureSRV(7, registry.GetTextureSRVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertiesDataBTexture));
                    shaderConstants.BindTextureUAV(8, registry.GetTextureUAVBindlessResourceDescriptorIndex(volumetricFogLightScatteringTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VolumetricFogLightScattering);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        RenderGraphTextureHandle volumetricFogFinalIntegrationTexture = renderGraph.CreateTexture(volumetricFogCommonTextureDesc, "VolumetricFogFinalIntegrationTexture");

        renderGraph.AddPass(
            std::format("VolumetricFogFinalIntegration (Compute, {}x{}x1)", volumetricFogTileCountX, volumetricFogTileCountY),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                volumetricFogLightScatteringTexture = builder.ReadTexture(volumetricFogLightScatteringTexture, RenderBackendResourceState::ShaderResource);
                volumetricFogFinalIntegrationTexture = builder.WriteTexture(volumetricFogFinalIntegrationTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = CeilDiv(volumetricFogTileCountX, 8);
                    uint32 threadGroupCountY = CeilDiv(volumetricFogTileCountY, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(volumetricFogShaderParameterBuffer));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(volumetricFogLightScatteringTexture));
                    shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndex(volumetricFogFinalIntegrationTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VolumetricFogFinalIntegration);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

      renderGraph.AddPass(
            std::format("VolumetricFogComposition (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                volumetricFogFinalIntegrationTexture = builder.ReadTexture(volumetricFogFinalIntegrationTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindRenderTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);
                //builder.BindDepthStencil(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::SrcAlpha;
                    graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
                    graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::RGB;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(volumetricFogShaderParameterBuffer));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(volumetricFogFinalIntegrationTexture));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::FullScreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::VolumetricFogComposition);

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

    void RealTimeRenderer::RenderLocalFogVolumes(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        uint32 localFogVolumeInstanceCount = uint32(view.scene->localFogVolumes.size());

        if (localFogVolumeInstanceCount == 0)
        {
            return;
        }

        LocalFogVolumeRenderData renderData = {};
        renderData.instanceCount = localFogVolumeInstanceCount;
        renderData.instanceData.resize(localFogVolumeInstanceCount);

        for (uint32 index = 0; index < localFogVolumeInstanceCount; index++)
        {
            LocalFogVolumeRenderObject* localFogVolume = view.scene->localFogVolumes[index];

            LocalFogVolumeInstanceData& instanceData = renderData.instanceData[index];
            instanceData.localToWorldMatrix = localFogVolume->transform;
            instanceData.emission = localFogVolume->emission;
        }

        if (!localFogVolumeInstanceDataBufferUpload)
        {
            RenderBackendBufferDesc localFogVolumeInstanceDataBufferDesc = RenderBackendBufferDesc::CreateUpload(sizeof(LocalFogVolumeInstanceData) * localFogVolumeInstanceCount);
            //localFogVolumeInstanceDataBufferDesc.flags |= RenderBackendBufferCreateFlags::CpuToGpu; // TODO
            localFogVolumeInstanceDataBufferUpload = renderBackend->CreateBuffer(&localFogVolumeInstanceDataBufferDesc, nullptr, "LocalFogVolumeInstanceDataBuffer");
            localFogVolumeInstanceDataBufferSize = sizeof(LocalFogVolumeInstanceData) * localFogVolumeInstanceCount;
        }
        else if (localFogVolumeInstanceDataBufferSize < sizeof(LocalFogVolumeInstanceData) * localFogVolumeInstanceCount)
        {
            renderBackend->ResizeBuffer(localFogVolumeInstanceDataBufferUpload, sizeof(LocalFogVolumeInstanceData) * localFogVolumeInstanceCount);
            localFogVolumeInstanceDataBufferSize = sizeof(LocalFogVolumeInstanceData) * localFogVolumeInstanceCount;
        }

        void* data = nullptr;
        renderBackend->MapBuffer(localFogVolumeInstanceDataBufferUpload, &data);
        memcpy(data, renderData.instanceData.data(), localFogVolumeInstanceDataBufferSize);
        renderBackend->UnmapBuffer(localFogVolumeInstanceDataBufferUpload);

        RenderGraphBufferDesc localFogVolumeInstanceDataBufferDesc = RenderGraphBufferDesc::CreateStructured(sizeof(LocalFogVolumeInstanceData), localFogVolumeInstanceCount);
        RenderGraphBufferHandle localFogVolumeInstanceDataBuffer = renderGraph.CreateBuffer(localFogVolumeInstanceDataBufferDesc, "LocalFogVolumeInstanceDataBuffer");

        renderGraph.AddPass(
            std::format("UpdateLocalFogVolumeData"),
            RenderGraphPassFlags::Copy,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.CopyBuffer(
                        localFogVolumeInstanceDataBufferUpload,
                        0,
                        registry.GetRenderBackendBufferHandle(localFogVolumeInstanceDataBuffer),
                        0,
                        localFogVolumeInstanceDataBufferSize);
                };
            });

        renderGraph.AddPass(
            std::format("LocalFogVolume (Graphics, {}x{})", renderResolution.width, renderResolution.height),
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
                    shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(localFogVolumeInstanceDataBuffer));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::LocalFogVolumeVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::LocalFogVolumePS);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderConstants,
                        36, localFogVolumeInstanceCount, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }
}