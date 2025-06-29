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
        float startingDistance;
        float endingDistance;
        Vector3f scattering;
        Vector3f absorption;
        Vector3f emission;
        Vector3f jitterOffsets[8];
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
        GlobalFogRenderObject* globalFog = view.scene->GetActiveGlobalFog();

        if (globalFog == nullptr)
        {
            return;
        }

        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "VolumetricFog");

        uint32 localFogVolumeInstanceCount = uint32(view.GetRenderScene()->localFogVolumes.size());
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
            RenderBackendBufferDescription localFogVolumeInstanceDataBufferDesc = RenderBackendBufferDescription::CreateUpload(sizeof(LocalFogVolumeInstanceData) * localFogVolumeInstanceCount);
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

        RenderGraphBufferDescription localFogVolumeInstanceDataBufferDescription = RenderGraphBufferDescription::CreateStructured(sizeof(LocalFogVolumeInstanceData), localFogVolumeInstanceCount);
        RenderGraphBufferHandle localFogVolumeInstanceDataBuffer = renderGraph.CreateBuffer(localFogVolumeInstanceDataBufferDescription, "LocalFogVolumeInstanceDataBuffer");

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
        const float startingDistance = perFrameShaderParameters.nearClippingPlane;
        const float endingDistance = std::max(startingDistance, globalFog->distance);

        uint32 volumetricFogShaderParameterBufferSize = sizeof(VolumetricFogShaderParameters);

        VolumetricFogShaderParameters volumetricFogShaderParameters = {};
        volumetricFogShaderParameters.volumeResolution = Vector3u(volumetricFogTileCountX, volumetricFogTileCountY, volumetricFogDepthSliceCount);
        volumetricFogShaderParameters.tileCountAndInverseTileCount = Vector4f(float(volumetricFogTileCountX), float(volumetricFogTileCountY), 1.0f / float(volumetricFogTileCountX), 1.0f / float(volumetricFogTileCountY));
        volumetricFogShaderParameters.sliceCountAndInverseSliceCount = Vector2f(float(volumetricFogDepthSliceCount), 1.0f / float(volumetricFogDepthSliceCount));
        volumetricFogShaderParameters.localFogVolumeInstanceCount = localFogVolumeInstanceCount;
        volumetricFogShaderParameters.startingDistance = startingDistance;
        volumetricFogShaderParameters.endingDistance = endingDistance;
        volumetricFogShaderParameters.scattering = globalFog->scattering;
        volumetricFogShaderParameters.absorption = globalFog->absorption;
        volumetricFogShaderParameters.emission = globalFog->emission;
        for (uint32 index = 0; index < 8; index++)
        {
            int32 phaseCount = 16;
            int32 haltonIndex = static_cast<int32>(perFrameShaderParameters.frameIndex + index + 1);
            volumetricFogShaderParameters.jitterOffsets[index] = Vector3f(Math::Halton(haltonIndex % phaseCount, 2), Math::Halton(haltonIndex % phaseCount, 3), Math::Halton(haltonIndex % phaseCount, 5));
        }

        static RenderBackendBufferHandle volumetricFogShaderParameterBufferUpload;
        if (!volumetricFogShaderParameterBufferUpload)
        {
            RenderBackendBufferDescription volumetricFogShaderParameterBufferUploadDescription = RenderBackendBufferDescription::CreateUpload(volumetricFogShaderParameterBufferSize);
            volumetricFogShaderParameterBufferUpload = renderBackend->CreateBuffer(&volumetricFogShaderParameterBufferUploadDescription, nullptr, "VolumetricFogShaderParameterBufferUpload");
        }
        renderBackend->UpdateBuffer(volumetricFogShaderParameterBufferUpload, 0, &volumetricFogShaderParameters, volumetricFogShaderParameterBufferSize);

        RenderGraphBufferDescription volumetricFogShaderParameterBufferDescription = RenderGraphBufferDescription::CreateStructured(volumetricFogShaderParameterBufferSize, 1);
        RenderGraphBufferHandle volumetricFogShaderParameterBuffer = renderGraph.CreateBuffer(volumetricFogShaderParameterBufferDescription, "VolumetricFogShaderParameterBuffer");

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

        RenderGraphTextureDescription volumetricFogCommonTextureDescription = RenderGraphTextureDescription::Create3D(
            volumetricFogTileCountX,
            volumetricFogTileCountY,
            volumetricFogDepthSliceCount,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);

        RenderGraphTextureHandle volumetricFogParticipatingMediaPropertiesDataATexture = renderGraph.CreateTexture(volumetricFogCommonTextureDescription, "VolumetricFogParticipatingPropertiesDataATexture");
        RenderGraphTextureHandle volumetricFogParticipatingMediaPropertiesDataBTexture = renderGraph.CreateTexture(volumetricFogCommonTextureDescription, "VolumetricFogParticipatingPropertiesDataBTexture");

        renderGraph.AddPass(
             std::format("VolumetricFogVoxelization (Compute, {}x{}x{})", volumetricFogTileCountX, volumetricFogTileCountY, volumetricFogDepthSliceCount),
             RenderGraphPassFlags::Compute,
             [&](RenderGraphBuilder& builder)
             {
                 builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                 builder.SetBindlessResourceSRV(1, volumetricFogShaderParameterBuffer);
                 builder.SetBindlessResourceSRV(2, localFogVolumeInstanceDataBuffer);
                 builder.SetBindlessResourceUAV(3, volumetricFogParticipatingMediaPropertiesDataATexture, 0);
                 builder.SetBindlessResourceUAV(4, volumetricFogParticipatingMediaPropertiesDataBTexture, 0);

                 RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::VolumetricFogVoxelization);

                 uint32 threadGroupCountX = ComputeShaderThreadGroupCount(volumetricFogTileCountX, 4);
                 uint32 threadGroupCountY = ComputeShaderThreadGroupCount(volumetricFogTileCountY, 4);
                 uint32 threadGroupCountZ = ComputeShaderThreadGroupCount(volumetricFogDepthSliceCount, 4);

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

        RenderGraphTextureHandle previousVolumetricFogLightScatteringTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        if (historyFrame.volumetricFogLightScatteringTexture != nullptr)
        {
            previousVolumetricFogLightScatteringTexture = renderGraph.ImportExternalTexture(historyFrame.volumetricFogLightScatteringTexture, "PreviousVolumetricFogLightScatteringTexture");
        }

        RenderGraphTextureHandle volumetricFogLightScatteringTexture = renderGraph.CreateTexture(volumetricFogCommonTextureDescription, "VolumetricFogLightScatteringTexture");

        renderGraph.AddPass(
            std::format("VolumetricFogLightScattering (Compute, {}x{}x{})", volumetricFogTileCountX, volumetricFogTileCountY, volumetricFogDepthSliceCount),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, volumetricFogShaderParameterBuffer);
                builder.SetBindlessResourceSRV(2, view.GetRenderScene()->distantLightDataBuffer);
                builder.SetBindlessResourceSRV(3, intermediateResources.cascadedShadowMapShaderParameterBuffer);
                builder.SetBindlessResourceSRV(4, intermediateResources.cascadedShadowMapDepthTexture);
                builder.SetBindlessResourceSRV(5, intermediateResources.irradianceEnvironmentMapBuffer);
                builder.SetBindlessResourceSRV(6, previousVolumetricFogLightScatteringTexture);
                builder.SetBindlessResourceSRV(7, volumetricFogParticipatingMediaPropertiesDataATexture);
                builder.SetBindlessResourceSRV(8, volumetricFogParticipatingMediaPropertiesDataBTexture);
                builder.SetBindlessResourceUAV(9, volumetricFogLightScatteringTexture, 0);

                RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::VolumetricFogLightScattering);

                uint32 threadGroupCountX = ComputeShaderThreadGroupCount(volumetricFogTileCountX, 4);
                uint32 threadGroupCountY = ComputeShaderThreadGroupCount(volumetricFogTileCountY, 4);
                uint32 threadGroupCountZ = ComputeShaderThreadGroupCount(volumetricFogDepthSliceCount, 4);

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

        renderGraph.ExportTextureDeferred(volumetricFogLightScatteringTexture, &historyFrame.volumetricFogLightScatteringTexture);

        RenderGraphTextureHandle volumetricFogFinalIntegrationTexture = renderGraph.CreateTexture(volumetricFogCommonTextureDescription, "VolumetricFogFinalIntegrationTexture");

        renderGraph.AddPass(
            std::format("VolumetricFogFinalIntegration (Compute, {}x{}x1)", volumetricFogTileCountX, volumetricFogTileCountY),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, volumetricFogShaderParameterBuffer);
                builder.SetBindlessResourceSRV(2, volumetricFogLightScatteringTexture);
                builder.SetBindlessResourceUAV(3, volumetricFogFinalIntegrationTexture, 0);

                RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::VolumetricFogFinalIntegration);

                uint32 threadGroupCountX = ComputeShaderThreadGroupCount(volumetricFogTileCountX, 8);
                uint32 threadGroupCountY = ComputeShaderThreadGroupCount(volumetricFogTileCountY, 8);
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

      renderGraph.AddPass(
            std::format("VolumetricFogComposition (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, volumetricFogShaderParameterBuffer);
                builder.SetBindlessResourceSRV(2, volumetricFogFinalIntegrationTexture);
                builder.SetBindlessResourceSRV(3, intermediateResources.depthTexture);
                builder.SetRenderTargetBinding(0, intermediateResources.colorTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);

                RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::DrawFullscreenQuadVS);
                RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::VolumetricFogComposition);

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

                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

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
            RenderBackendBufferDescription localFogVolumeInstanceDataBufferDescription = RenderBackendBufferDescription::CreateUpload(sizeof(LocalFogVolumeInstanceData) * localFogVolumeInstanceCount);
            //localFogVolumeInstanceDataBufferDesc.flags |= RenderBackendBufferCreateFlags::CpuToGpu; // TODO
            localFogVolumeInstanceDataBufferUpload = renderBackend->CreateBuffer(&localFogVolumeInstanceDataBufferDescription, nullptr, "LocalFogVolumeInstanceDataBuffer");
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

        RenderGraphBufferDescription localFogVolumeInstanceDataBufferDescription = RenderGraphBufferDescription::CreateStructured(sizeof(LocalFogVolumeInstanceData), localFogVolumeInstanceCount);
        RenderGraphBufferHandle localFogVolumeInstanceDataBuffer = renderGraph.CreateBuffer(localFogVolumeInstanceDataBufferDescription, "LocalFogVolumeInstanceDataBuffer");

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

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(localFogVolumeInstanceDataBuffer));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));

                    RenderBackendShaderHandle vertexShader = shaderCollection->GetShader(ShaderID::LocalFogVolumeVS);
                    RenderBackendShaderHandle pixelShader = shaderCollection->GetShader(ShaderID::LocalFogVolumePS);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        pushConstantValues,
                        36, localFogVolumeInstanceCount, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }
}