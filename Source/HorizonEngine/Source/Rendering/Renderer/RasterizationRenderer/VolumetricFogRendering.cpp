#include "RasterizationRenderer.h"
#include "Engine/Core/Scene.h"

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

    void RasterizationRenderer::RenderVolumetricFog(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        uint32 localFogVolumeInstanceCount = uint32(view.GetRenderScene()->localFogVolumes.size());

        if (localFogVolumeInstanceCount == 0)
        {
            return;
        }

        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "VolumetricFog");

        LocalFogVolumeRenderData renderData = {};
        renderData.instanceCount = localFogVolumeInstanceCount;
        renderData.instanceData.resize(localFogVolumeInstanceCount);

        for (uint32 index = 0; index < localFogVolumeInstanceCount; index++)
        {
            LocalFogVolumeRenderObject* localFogVolume = view.GetRenderScene()->localFogVolumes[index];

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
                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.CopyBuffer(
                        localFogVolumeInstanceDataBufferUpload,
                        0,
                        resourceRegistry.GetRenderBackendBufferHandle(localFogVolumeInstanceDataBuffer),
                        0,
                        localFogVolumeInstanceDataBufferSize);
                };
            });

        const uint32 volumetricFogTileSize = VolumetricFogTileSize;
        const uint32 volumetricFogTileCountX = Math::CeilDiv(renderResolution.width, volumetricFogTileSize);
        const uint32 volumetricFogTileCountY = Math::CeilDiv(renderResolution.height, volumetricFogTileSize);
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

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        renderGraph.AddPass(
            std::format("UpdateVolumetricFog"),
            RenderGraphPassFlags::Copy,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.CopyBuffer(
                        volumetricFogShaderParameterBufferUpload,
                        0,
                        resourceRegistry.GetRenderBackendBufferHandle(volumetricFogShaderParameterBuffer),
                        0,
                        volumetricFogShaderParameterBufferSize);
                };
            });

        RenderGraphTextureDescription volumetricFogCommonTextureDesc = RenderGraphTextureDescription::Create3D(
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

                 return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                 {
                     uint32 threadGroupCountX = ComputeShaderThreadGroupCount(volumetricFogTileCountX, 4);
                     uint32 threadGroupCountY = ComputeShaderThreadGroupCount(volumetricFogTileCountY, 4);
                     uint32 threadGroupCountZ = ComputeShaderThreadGroupCount(volumetricFogDepthSliceCount, 4);

                     RenderBackendPushConstantValues shaderConstants = {};
                     shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                     shaderConstants.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(volumetricFogShaderParameterBuffer));
                     shaderConstants.BindBufferSRV(2, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(localFogVolumeInstanceDataBuffer));
                     shaderConstants.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertiesDataATexture, 0));
                     shaderConstants.BindTextureUAV(4, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertiesDataBTexture, 0));

                     RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::VolumetricFogVoxelization);

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
                RenderGraphTextureHandle shadowMapTexture = builder.ReadTexture(intermediateResources.cascadedShadowMapDepthTexture, RenderBackendResourceState::ShaderResource);
                volumetricFogParticipatingMediaPropertiesDataATexture = builder.ReadTexture(volumetricFogParticipatingMediaPropertiesDataATexture, RenderBackendResourceState::ShaderResource);
                volumetricFogParticipatingMediaPropertiesDataBTexture = builder.ReadTexture(volumetricFogParticipatingMediaPropertiesDataBTexture, RenderBackendResourceState::ShaderResource);
                previousVolumetricFogLightScatteringTexture = builder.ReadTexture(previousVolumetricFogLightScatteringTexture, RenderBackendResourceState::ShaderResource);
                volumetricFogLightScatteringTexture = builder.WriteTexture(volumetricFogLightScatteringTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(volumetricFogTileCountX, 4);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(volumetricFogTileCountY, 4);
                    uint32 threadGroupCountZ = ComputeShaderThreadGroupCount(volumetricFogDepthSliceCount, 4);

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(volumetricFogShaderParameterBuffer));
                    shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(view.GetRenderScene()->distantLightDataBuffer));
                    shaderConstants.BindBufferSRV(3, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(intermediateResources.cascadedShadowMapShaderParameterBuffer));
                    shaderConstants.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(shadowMapTexture));
                    shaderConstants.BindTextureSRV(5, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(previousVolumetricFogLightScatteringTexture));
                    shaderConstants.BindTextureSRV(6, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertiesDataATexture));
                    shaderConstants.BindTextureSRV(7, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(volumetricFogParticipatingMediaPropertiesDataBTexture));
                    shaderConstants.BindTextureUAV(8, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(volumetricFogLightScatteringTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::VolumetricFogLightScattering);

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

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(volumetricFogTileCountX, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(volumetricFogTileCountY, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(volumetricFogShaderParameterBuffer));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(volumetricFogLightScatteringTexture));
                    shaderConstants.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(volumetricFogFinalIntegrationTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::VolumetricFogFinalIntegration);

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
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneColorTexture = intermediateResources.colorTexture = builder.WriteTexture(intermediateResources.colorTexture, RenderBackendResourceState::RenderTarget);

                builder.SetRenderTargetBinding(0, sceneColorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::NotEqual;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::SrcAlpha;
                    graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
                    graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::RGB;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(volumetricFogShaderParameterBuffer));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(volumetricFogFinalIntegrationTexture));
                    shaderConstants.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));

                    RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::DrawFullscreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::VolumetricFogComposition);

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

    void RasterizationRenderer::RenderLocalFogVolumes(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        uint32 localFogVolumeInstanceCount = uint32(view.GetRenderScene()->localFogVolumes.size());

        if (localFogVolumeInstanceCount == 0)
        {
            return;
        }

        LocalFogVolumeRenderData renderData = {};
        renderData.instanceCount = localFogVolumeInstanceCount;
        renderData.instanceData.resize(localFogVolumeInstanceCount);

        for (uint32 index = 0; index < localFogVolumeInstanceCount; index++)
        {
            LocalFogVolumeRenderObject* localFogVolume = view.GetRenderScene()->localFogVolumes[index];

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
                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.CopyBuffer(
                        localFogVolumeInstanceDataBufferUpload,
                        0,
                        resourceRegistry.GetRenderBackendBufferHandle(localFogVolumeInstanceDataBuffer),
                        0,
                        localFogVolumeInstanceDataBufferSize);
                };
            });

        renderGraph.AddPass(
            std::format("LocalFogVolume (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

                RenderGraphTextureHandle sceneColorTexture = intermediateResources.colorTexture = builder.WriteTexture(intermediateResources.colorTexture, RenderBackendResourceState::RenderTarget);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::DepthStencilReadOnly);

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

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(localFogVolumeInstanceDataBuffer));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));

                    RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::LocalFogVolumeVS);
                    RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::LocalFogVolumePS);

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