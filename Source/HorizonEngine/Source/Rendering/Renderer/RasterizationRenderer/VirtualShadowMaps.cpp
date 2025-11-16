#include "VirtualShadowMaps.h"
#include "RasterizationRenderer.h"

namespace Horizon
{
    uint32 VirtualShadowMapPageSize = 128;
    uint32 VirtualShadowMapClipmapFirstLevel = 1;
    uint32 VirtualShadowMapClipmapLevelCount = 8;

    static float ComputeClipmapLevelRadius(uint32 level)
    {
        return std::pow(2.0f, float(level + 1));
    }

    VirtualShadowMapClipmap* VirtualShadowMapManager::CreateVirtualShadowMapClipmap(
        const SceneView& view,
        const LightRenderObject& light)
    {
        uint32 firstLevel = VirtualShadowMapClipmapFirstLevel;
        uint32 levelCount = VirtualShadowMapClipmapLevelCount;
        VirtualShadowMapClipmap* virtualShadowMapClipmap = new VirtualShadowMapClipmap();
        virtualShadowMapClipmap->virtualShadowMapIndex = virtualShadowMapCount;
        virtualShadowMapClipmap->lightDirection = light.GetDirection();
        virtualShadowMapClipmap->firstLevel = firstLevel;
        virtualShadowMapClipmap->levelCount = levelCount;
        virtualShadowMapClipmap->virtualShadowMapEntryIDBase = virtualShadowMapEntryCount;

        for (uint32 index = 0; index < levelCount; index++)
        {
            uint32 level = firstLevel + index;
            float radius = ComputeClipmapLevelRadius(level);
            float near = -radius;
            float far = radius;

            Vector3f worldSpaceOrigin = view.cameraPosition;

            VirtualShadowMapClipmap::ClipmapLevelData& levelData = virtualShadowMapClipmap->levels.emplace_back();
            levelData.origin = worldSpaceOrigin;
            levelData.viewToClipMatrix = Math::OrthographicProjection_ReverseZ_ZO(-radius, radius, -radius, radius, near, far);

            Matrix4x4f worldToViewMatrix = glm::lookAt(worldSpaceOrigin, worldSpaceOrigin + virtualShadowMapClipmap->lightDirection, glm::vec3(0.0f, 1.0f, 0.0f));
            VirtualShadowMapEntryShaderParameters& virtualShadowMapEntryShaderParameters = virtualShadowMapEntries.emplace_back();
            virtualShadowMapEntryShaderParameters.worldToClipMatrix = levelData.viewToClipMatrix * worldToViewMatrix;
            virtualShadowMapEntryShaderParameters.worldSpaceOrigin = Vector4f(worldSpaceOrigin.x, worldSpaceOrigin.y, worldSpaceOrigin.z, 1.0f);
            virtualShadowMapEntryShaderParameters.lightType = uint32(LightType::DistantLight);
            virtualShadowMapEntryShaderParameters.level = level;
            virtualShadowMapEntryShaderParameters.virtualShadowMapEntryIndex = virtualShadowMapClipmap->virtualShadowMapEntryIDBase + index;
        }

        virtualShadowMaps.push_back(virtualShadowMapClipmap);

        virtualShadowMapCount += 1;
        virtualShadowMapEntryCount += VirtualShadowMapClipmapLevelCount;

        return virtualShadowMapClipmap;
    }

    void VirtualShadowMapManager::Clear()
    {
        for (uint32 index = 0; index < virtualShadowMapCount; index++)
        {
            delete virtualShadowMaps[index];
        }
        virtualShadowMaps.clear();
        virtualShadowMapCount = 0;
        virtualShadowMapEntries.clear();
        virtualShadowMapEntryCount = 0;
    }

    void RasterizationRenderer::DispatchVirtualShadowMapPassDrawCommands(
        RenderBackendCommandList& commandList,
        const LightRenderObject& light,
        RenderBackendBufferHandle virtualShadowMapShaderParameterBuffer,
        RenderBackendBufferHandle virtualShadowMapPageTableBuffer,
        RenderBackendBufferHandle virtualShadowMapEntryBuffer,
        RenderBackendTextureHandle virtualShadowMapDepthTexture)
    {
        // const GeometryPassDrawCommandList& drawCommandList = geometryPassDrawCommandLists[uint32(GeometryPassType::VirtualShadowMap)];
        // const GPUScene* gpuScene = sceneView->scene->GetGPUScene();//drawCommandList.setupJobData.scene->GetGPUScene();
        //
        // RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::VirtualShadowMapDepthVS);
        // RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::VirtualShadowMapDepthPS);
        //
        // for (uint32 drawCommandIndex = 0; drawCommandIndex < drawCommandList.drawCommandCount; drawCommandIndex++)
        // {
        //     const GeometryPassDrawCommand& drawCommand = drawCommandList.commands[drawCommandIndex];
        //
        //     RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
        //     graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
        //     graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
        //     graphicsPipelineState.depthStencilState.depthTestEnable = false; // Disable hardware depth testing
        //     graphicsPipelineState.depthStencilState.depthWriteEnable = false;
        //     graphicsPipelineState.depthStencilState.stencilTestEnable = false;
        //
        //     RenderBackendPushConstantValues pushConstantValues = {};
        //     pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
        //     pushConstantValues.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
        //     pushConstantValues.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapPageTableBuffer));
        //     pushConstantValues.BindBufferSRV(3, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
        //     pushConstantValues.BindBufferSRV(4, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
        //     pushConstantValues.BindTextureUAV(5, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(virtualShadowMapDepthTexture, 0));
        //     pushConstantValues.BindTextureSRV(6, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapEntryBuffer));
        //
        //     for (uint32 shadowViewIndex = 0; shadowViewIndex < virtualShadowMapManager->GetVirtualShadowMapEntryCount(); shadowViewIndex++)
        //     {
        //         pushConstantValues.OverrideShaderConstantValue(7, shadowViewIndex);
        //
        //         commandList.DrawIndexed(
        //             vertexShader,
        //             pixelShader,
        //             graphicsPipelineState,
        //             pushConstantValues,
        //             drawCommand.indexBuffer,
        //             drawCommand.indexCount,
        //             drawCommand.instanceCount,
        //             drawCommand.firstIndex,
        //             0, // TODO
        //             drawCommand.firstInstance,
        //             drawCommand.topology);
        //     }
        // }
    }

    void SetupVirtualShadowMapShaderParameters(VirtualShadowMapShaderParameters& outParameters, const SceneView& view, const LightRenderObject& light)
    {
        const Vector3f& lightDirection = light.GetDirection();
        const uint32 shadowCascadeCount = 1;//light.GetShadowCascadeCount();
        const float maxShadowDistance = light.GetMaxShadowDistance();

        //Matrix4x4f worldToLightMatrix = light.color;
        float shadowMapSize = 128.0f * 128.0f;

        float cascadeSplits[RendererMaxShadowMapCascadeCount];

        float nearClip = view.nearClippingPlane;
        float farClip = maxShadowDistance;//camera.farClippingPlane;
        float fieldOfView = view.verticalFOV;
        float aspectRatio = view.aspectRatio;

        float clipRange = farClip - nearClip;

        float minZ = nearClip;
        float maxZ = nearClip + clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        // Calculate split plane based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
        for (uint32 i = 0; i < shadowCascadeCount; i++)
        {
            float p = (i + 1) / static_cast<float>(shadowCascadeCount);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = light.GetShadowCascadeSplitLambda() * (log - uniform) + uniform;
            cascadeSplits[i] = (d - nearClip) / clipRange;
        }

        // Calculate orthographic projection matrix for each cascade
        float lastSplitDist = 0.0;
        for (uint32 cascadeIndex = 0; cascadeIndex < shadowCascadeCount; cascadeIndex++)
        {
            float splitDist = cascadeSplits[cascadeIndex];

            glm::vec3 frustumCorners[8] =
            {
                glm::vec3(-1.0f,  1.0f,  1.0f),
                glm::vec3(1.0f,  1.0f,  1.0f),
                glm::vec3(1.0f, -1.0f,  1.0f),
                glm::vec3(-1.0f, -1.0f,  1.0f),
                glm::vec3(-1.0f,  1.0f,  0.0f),
                glm::vec3(1.0f,  1.0f,  0.0f),
                glm::vec3(1.0f, -1.0f,  0.0f),
                glm::vec3(-1.0f, -1.0f,  0.0f),
            };

            // Project frustum corners into world space
            glm::mat4 cameraProjectionMatrix = Math::PerspectiveProjection_ReverseZ_RH_ZO(fieldOfView, aspectRatio, nearClip, farClip);
            glm::mat4 inverseCameraProjectionMatrix = Math::InverseMatrix(cameraProjectionMatrix);

            glm::mat4 invCam = view.transformations.viewToWorldMatrix * inverseCameraProjectionMatrix;
            for (uint32 i = 0; i < 8; i++)
            {
                glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
                frustumCorners[i] = invCorner / invCorner.w;
            }

            for (uint32 i = 0; i < 4; i++)
            {
                glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
                frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
                frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
            }

            // Get frustum position
            glm::vec3 frustumCenter = glm::vec3(0.0f);
            for (uint32 i = 0; i < 8; i++)
            {
                frustumCenter += frustumCorners[i];
            }
            frustumCenter /= 8.0f;

            float radius = 0.0f;
            for (uint32 i = 0; i < 8; i++)
            {
                float distance = glm::length(frustumCorners[i] - frustumCenter);
                radius = glm::max(radius, distance);
            }
            radius = std::ceil(radius);

            glm::mat viewMatrix = glm::lookAt(view.cameraPosition, view.cameraPosition + lightDirection, glm::vec3(0.0f, 1.0f, 0.0f));

            float far = radius;
            float near = -radius;

            glm::mat projectionMatrix = Math::OrthographicProjection_ReverseZ_ZO(-radius, radius, -radius, radius, near, far);

            glm::mat4 shadowMatrix = projectionMatrix * viewMatrix;
            glm::vec4 shadowOrigin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            shadowOrigin = shadowMatrix * shadowOrigin;
            float storedW = shadowOrigin.w;
            shadowOrigin = shadowOrigin * shadowMapSize / 2.0f;

            glm::vec4 roundedOrigin = glm::round(shadowOrigin);
            glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
            roundOffset = roundOffset * 2.0f / shadowMapSize;
            roundOffset.z = 0.0f;
            roundOffset.w = 0.0f;

            glm::mat4 shadowProj = projectionMatrix;
            shadowProj[3] += roundOffset;
            projectionMatrix = shadowProj;

            lastSplitDist = cascadeSplits[cascadeIndex];

            outParameters.physicalPageCount = 128 * 128;
            outParameters.worldToClipMatrix = projectionMatrix * viewMatrix;
        }
    }

    void RasterizationRenderer::RenderVirtualShadowMapDepth(RenderGraph& renderGraph, const SceneView& view)
    {
        const LightRenderObject* light = nullptr;
        for (uint32 lightIndex = 0; lightIndex < uint32(view.scene->lights.size()); lightIndex++)
        {
            light = view.scene->lights[lightIndex];
            if (light->lightType == LightType::DistantLight)
            {
                break;
            }
        }

        if (!light)
        {
            return;
        }

        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "VirtualShadowMaps");

        const uint32 virtualShadowMapSize = 128 * 128;
        const uint32 virtualShadowMapEntryCount = virtualShadowMapManager->GetVirtualShadowMapEntryCount();
        const uint32 virtualShadowMapMaximumVirtualPageCount = virtualShadowMapEntryCount * 128 * 128;

        VirtualShadowMapShaderParameters virtualShadowMapShaderParameters;
        SetupVirtualShadowMapShaderParameters(virtualShadowMapShaderParameters, view, *light);
        virtualShadowMapShaderParameters.maximumVirtualPageCount = virtualShadowMapMaximumVirtualPageCount;

        const uint32 virtualShadowMapPhysicalPageCount = virtualShadowMapShaderParameters.physicalPageCount;

        RenderBackendBufferHandle& virtualShadowMapShaderParameterUploadBuffer = virtualShadowMapShaderParameterUploadBuffers[currentPerFrameDataBufferIndex];
        RenderBackendBufferHandle& virtualShadowMapShaderParameterBuffer = virtualShadowMapShaderParameterBuffers[currentPerFrameDataBufferIndex];
        if (!virtualShadowMapShaderParameterBuffer)
        {
            RenderBackendBufferDescription virtualShadowMapShaderParameterUploadBufferDesc = RenderBackendBufferDescription::CreateUpload(sizeof(VirtualShadowMapShaderParameters));
            virtualShadowMapShaderParameterUploadBuffer = renderBackend->CreateBuffer(&virtualShadowMapShaderParameterUploadBufferDesc, nullptr, "VirtualShadowMapShaderParameterUploadBuffer");
            RenderBackendBufferDescription virtualShadowMapShaderParameterBufferDesc = RenderBackendBufferDescription::CreateStructured(sizeof(VirtualShadowMapShaderParameters), 1);
            virtualShadowMapShaderParameterBuffer = renderBackend->CreateBuffer(&virtualShadowMapShaderParameterBufferDesc, nullptr, "VirtualShadowMapShaderParameterBuffer");
        }
        renderBackend->UpdateBuffer(virtualShadowMapShaderParameterUploadBuffer, 0, &virtualShadowMapShaderParameters, sizeof(VirtualShadowMapShaderParameters));

        renderGraph.AddPass(
            std::format("UpdateVirtualShadowMapShaderParameterBuffer (Copy, {} bytes)", sizeof(VirtualShadowMapShaderParameters)),
            RenderGraphPassFlags::Copy,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.CopyBuffer(
                        virtualShadowMapShaderParameterUploadBuffer,
                        0,
                        virtualShadowMapShaderParameterBuffer,
                        0,
                        sizeof(VirtualShadowMapShaderParameters));
                };
            });

        uint32 virtualShadowMapEntryBufferSize = virtualShadowMapEntryCount * sizeof(VirtualShadowMapEntryShaderParameters);
        RenderGraphBufferHandle virtualShadowMapEntryBuffer = renderGraph.CreateBuffer(RenderGraphBufferDescription::CreateByteAddress(virtualShadowMapEntryBufferSize), "VirtualShadowMapEntryBuffer");
        renderGraph.UploadBufferDeferred(virtualShadowMapEntryBuffer, virtualShadowMapManager->virtualShadowMapEntries.data(), virtualShadowMapEntryBufferSize, RenderGraphSourceDataLifetimeHint::ValidUntilExecution);

        uint32 distantLightVirtualShadowMapEntryCount = 0;
        std::vector<uint32> distantLightVirtualShadowMapEntries;
        for (const auto& virtualShadowMapEntry : virtualShadowMapManager->virtualShadowMapEntries)
        {
            if (virtualShadowMapEntry.lightType == uint32(LightType::DistantLight))
            {
                distantLightVirtualShadowMapEntries.push_back(distantLightVirtualShadowMapEntryCount);
                distantLightVirtualShadowMapEntryCount++;
            }
        }

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        RenderGraphBufferDescription virtualShadowMapPageRequestBufferDesc = RenderGraphBufferDescription::CreateByteAddress(virtualShadowMapMaximumVirtualPageCount * sizeof(uint32));
        RenderGraphBufferHandle virtualShadowMapPageRequestBuffer = renderGraph.CreateBuffer(virtualShadowMapPageRequestBufferDesc, "VirtualShadowMapPageRequestBuffer");

        RenderGraphBufferDescription virtualShadowMapPageTableBufferDesc = RenderGraphBufferDescription::CreateByteAddress(virtualShadowMapMaximumVirtualPageCount * sizeof(uint32));
        RenderGraphBufferHandle virtualShadowMapPageTableBuffer = renderGraph.CreateBuffer(virtualShadowMapPageTableBufferDesc, "VirtualShadowMapPageTableBuffer");

        RenderGraphBufferDescription virtualShadowMapPhysicalPageDataBufferDesc = RenderGraphBufferDescription::CreateByteAddress(virtualShadowMapPhysicalPageCount * sizeof(uint32));
        RenderGraphBufferHandle virtualShadowMapPhysicalPageDataBuffer = renderGraph.CreateBuffer(virtualShadowMapPhysicalPageDataBufferDesc, "VirtualShadowMapPhysicalPageDataBuffer");

        RenderGraphBufferDescription virtualShadowMapPhysicalPageListBufferDesc = RenderGraphBufferDescription::CreateByteAddress(sizeof(uint32), 1);
        RenderGraphBufferHandle virtualShadowMapPhysicalPageListBuffer = renderGraph.CreateBuffer(virtualShadowMapPhysicalPageListBufferDesc, "VirtualShadowMapPhysicalPageListBuffer");

        RenderGraphBufferDescription virtualShadowMapIndirectArgumentBufferDesc = RenderGraphBufferDescription::CreateIndirectArguments(sizeof(uint32), 3);
        RenderGraphBufferHandle virtualShadowMapIndirectArgumentBuffer = renderGraph.CreateBuffer(virtualShadowMapIndirectArgumentBufferDesc, "VirtualShadowMapIndirectArgumentBuffer");

        RenderGraphBufferDescription virtualShadowMapActivePhysicalPageIndexBufferDesc = RenderGraphBufferDescription::CreateByteAddress(virtualShadowMapPhysicalPageCount * sizeof(uint32));
        RenderGraphBufferHandle virtualShadowMapActivePhysicalPageIndexBuffer = renderGraph.CreateBuffer(virtualShadowMapActivePhysicalPageIndexBufferDesc, "VirtualShadowMapActivePhysicalPageIndexBuffer");

        RenderGraphTextureDescription virtualShadowMapDepthTextureDesc = RenderGraphTextureDescription::Create2DArray(
            virtualShadowMapSize,
            virtualShadowMapSize,
            RenderBackendTextureFormat::R32Uint,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            1,
            RenderBackendTextureClearValue::Black,
            1,
            1,
            RenderBackendResourceState::UnorderedAccess);

        RenderGraphTextureHandle virtualShadowMapDepthTexture = intermediateResources.virtualShadowMapDepthTexture = renderGraph.CreateTexture(virtualShadowMapDepthTextureDesc, "VirtualShadowMapDepthTexture");

        renderGraph.AddPass(
            std::format("VirtualShadowMapClearPageDataBuffer ({} bytes)", virtualShadowMapPhysicalPageDataBufferDesc.size),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapPageRequestBuffer = builder.WriteBuffer(virtualShadowMapPageRequestBuffer, RenderBackendResourceState::UnorderedAccess);
                virtualShadowMapPhysicalPageDataBuffer = builder.WriteBuffer(virtualShadowMapPhysicalPageDataBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.ClearBufferUAV(resourceRegistry.GetRenderBackendBufferHandle(virtualShadowMapPageRequestBuffer), 0);
                    commandList.ClearBufferUAV(resourceRegistry.GetRenderBackendBufferHandle(virtualShadowMapPhysicalPageDataBuffer), 0);
                };
            });

        renderGraph.AddPass(
            std::format("VirtualShadowMapClearIndirectArgumentBuffer (Compute, 1x1x1)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapIndirectArgumentBuffer = builder.WriteBuffer(virtualShadowMapIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);
                virtualShadowMapPhysicalPageListBuffer = builder.WriteBuffer(virtualShadowMapPhysicalPageListBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = 1;
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferUAV(0, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapIndirectArgumentBuffer));
                    pushConstantValues.BindBufferUAV(1, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapPhysicalPageListBuffer));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VirtualShadowMapClearIndirectArgumentBuffer);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("VirtualShadowMapClearPageTable (Compute, {}x1x1)", virtualShadowMapMaximumVirtualPageCount),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapPageTableBuffer = builder.WriteBuffer(virtualShadowMapPageTableBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(virtualShadowMapMaximumVirtualPageCount, 64);
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    pushConstantValues.BindBufferUAV(1, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapPageTableBuffer));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VirtualShadowMapClearPageTable);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("VirtualShadowMapPageRequest (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::ShaderResource);
                virtualShadowMapEntryBuffer = builder.ReadBuffer(virtualShadowMapEntryBuffer, RenderBackendResourceState::ShaderResource);
                virtualShadowMapPageRequestBuffer = builder.WriteBuffer(virtualShadowMapPageRequestBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    pushConstantValues.BindBufferUAV(3, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapPageRequestBuffer));
                    pushConstantValues.BindBufferSRV(4, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapEntryBuffer));
                    pushConstantValues.OverrideShaderConstantValue(5, distantLightVirtualShadowMapEntryCount);

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VirtualShadowMapPageRequest);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("VirtualShadowMapPhysicalPageAllocation (Compute, {}x1x1)", virtualShadowMapMaximumVirtualPageCount),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapPageRequestBuffer = builder.ReadBuffer(virtualShadowMapPageRequestBuffer, RenderBackendResourceState::ShaderResource);
                virtualShadowMapPhysicalPageListBuffer = builder.WriteBuffer(virtualShadowMapPhysicalPageListBuffer, RenderBackendResourceState::UnorderedAccess);
                virtualShadowMapPageTableBuffer = builder.WriteBuffer(virtualShadowMapPageTableBuffer, RenderBackendResourceState::UnorderedAccess);
                virtualShadowMapPhysicalPageDataBuffer = builder.WriteBuffer(virtualShadowMapPhysicalPageDataBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(virtualShadowMapMaximumVirtualPageCount, 64);
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    pushConstantValues.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapPageRequestBuffer));
                    pushConstantValues.BindBufferUAV(2, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapPhysicalPageListBuffer));
                    pushConstantValues.BindBufferUAV(3, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapPageTableBuffer));
                    pushConstantValues.BindBufferUAV(4, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapPhysicalPageDataBuffer));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VirtualShadowMapPhysicalPageAllocation);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("VirtualShadowMapPhysicalMemoryAllocation (Compute, {}x1x1)", virtualShadowMapPhysicalPageCount),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapPhysicalPageDataBuffer = builder.ReadBuffer(virtualShadowMapPhysicalPageDataBuffer, RenderBackendResourceState::ShaderResource);
                virtualShadowMapIndirectArgumentBuffer = builder.WriteBuffer(virtualShadowMapIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);
                virtualShadowMapActivePhysicalPageIndexBuffer = builder.WriteBuffer(virtualShadowMapActivePhysicalPageIndexBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(virtualShadowMapPhysicalPageCount, 64);
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    pushConstantValues.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapPhysicalPageDataBuffer));
                    pushConstantValues.BindBufferUAV(2, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapIndirectArgumentBuffer));
                    pushConstantValues.BindBufferUAV(3, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapActivePhysicalPageIndexBuffer));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VirtualShadowMapPhysicalMemoryAllocation);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

#if 1
        renderGraph.AddPass(
            std::format("VirtualShadowMapClearPhysicalMemory (Compute, Indirect)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapIndirectArgumentBuffer = builder.ReadBuffer(virtualShadowMapIndirectArgumentBuffer, RenderBackendResourceState::IndirectArgument);
                virtualShadowMapDepthTexture = builder.WriteTexture(intermediateResources.virtualShadowMapDepthTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    pushConstantValues.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapActivePhysicalPageIndexBuffer));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(virtualShadowMapDepthTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VirtualShadowMapClearPhysicalMemory);

                    commandList.DispatchIndirect(
                        computeShader,
                        pushConstantValues,
                        resourceRegistry.GetRenderBackendBufferHandle(virtualShadowMapIndirectArgumentBuffer),
                        0);
                };
            });
#else
        renderGraph.AddPass(
            std::format("VirtualShadowMapClearPhysicalMemory (Compute, Indirect)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapDepthTexture = builder.WriteTexture(intermediateResources.virtualShadowMapDepthTexture, RenderBackendResourceState::UnorderedAccess);


                {
                    commandList.ClearTextureUAV(
                        RenderBackendTextureUAVDesc(resourceRegistry.GetRenderBackendTextureHandle(virtualShadowMapDepthTexture), 0),
                        RenderBackendTextureClearValue::Black);
                };
            });
#endif

        RenderGraphBufferDescription meshletCullingArgumentBufferDesc = RenderGraphBufferDescription::CreateIndirectArguments(sizeof(RenderBackendDispatchIndirectArguments), 1);
        RenderGraphBufferHandle meshletCullingArgumentBuffer = renderGraph.CreateBuffer(meshletCullingArgumentBufferDesc, "VirtualGeometryIndirectArgumentBuffer");

        RenderGraphBufferDescription drawIndirectArgumentBufferDesc = RenderGraphBufferDescription::CreateIndirectArguments(sizeof(RenderBackendDrawIndirectArguments), 1);
        RenderGraphBufferHandle drawIndirectArgumentBuffer = renderGraph.CreateBuffer(drawIndirectArgumentBufferDesc, "VirtualGeometryDrawIndirectArgumentBuffer");

        RenderGraphBufferDescription candidateVisibleMeshletBufferDesc = RenderGraphBufferDescription::CreateByteAddress(sizeof(VisibleMeshletEntry) * MaximumCandidateVisibleMeshletCount);
        RenderGraphBufferHandle candidateVisibleMeshletBuffer = renderGraph.CreateBuffer(candidateVisibleMeshletBufferDesc, "CandidateVisibleMeshletBuffer");

        RenderGraphBufferDescription visibleMeshletBufferDesc = RenderGraphBufferDescription::CreateByteAddress(sizeof(VisibleMeshletEntry) * MaximumVisibleMeshletCount);
        RenderGraphBufferHandle visibleMeshletBuffer = renderGraph.CreateBuffer(visibleMeshletBufferDesc, "VisibleMeshletBuffer");

        RenderGraphBufferDescription visibleMeshletCounterBufferDesc = RenderGraphBufferDescription::CreateByteAddress(sizeof(uint32));
        RenderGraphBufferHandle visibleMeshletCounterBuffer = renderGraph.CreateBuffer(visibleMeshletCounterBufferDesc, "VisibleMeshletCounterBuffer");

        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();

        renderGraph.AddPass(
            std::format("ClearVisibleMeshletCounterBuffer ({} bytes)", visibleMeshletCounterBufferDesc.size),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                visibleMeshletCounterBuffer = builder.WriteBuffer(visibleMeshletCounterBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.ClearBufferUAV(resourceRegistry.GetRenderBackendBufferHandle(visibleMeshletCounterBuffer), 0);
                };
            });

        renderGraph.AddPass(
            std::format("VisibilityCullingIndirectArgumentInitialization (Compute, 1x1x1)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                meshletCullingArgumentBuffer = builder.WriteBuffer(meshletCullingArgumentBuffer, RenderBackendResourceState::UnorderedAccess);
                drawIndirectArgumentBuffer = builder.WriteBuffer(drawIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VisibilityCullingIndirectArgumentInitialization);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferUAV(0, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(meshletCullingArgumentBuffer));
                    pushConstantValues.BindBufferUAV(1, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(drawIndirectArgumentBuffer));

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        1,
                        1,
                        1);
                };
            });

        RenderGraphBufferHandle geometryDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryDataBuffer);
        RenderGraphBufferHandle geometryInstanceDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryInstanceDataBuffer);
        const uint32 geometryInstanceCount = gpuScene->geometryInstanceCount;
        if (geometryInstanceCount > 0)
        {
            renderGraph.AddPass(
                std::format("VirtualShadowMapInstanceCulling (Compute)"),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    meshletCullingArgumentBuffer = builder.WriteBuffer(meshletCullingArgumentBuffer, RenderBackendResourceState::UnorderedAccess);

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VirtualShadowMapInstanceCulling);

                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(geometryInstanceCount, 64);
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        pushConstantValues.BindBufferSRV(1, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryDataBuffer));
                        pushConstantValues.BindBufferSRV(2, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryInstanceDataBuffer));
                        pushConstantValues.BindBufferUAV(3, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(meshletCullingArgumentBuffer));
                        pushConstantValues.OverrideShaderConstantValue(4, geometryInstanceCount);

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    };
                });
        }

        RenderGraphTextureHandle previousMinDepthPyramidTexture = renderGraph.ImportExternalTexture(historyFrame.minDepthPyramidTexture, "PreviousMinDepthPyramidTexture");
        const bool skipOcclusionCulling = previousMinDepthPyramidTexture.IsNull();

        renderGraph.AddPass(
            std::format("VirtualShadowMapMeshletCulling (Compute, Indirect)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                meshletCullingArgumentBuffer = builder.ReadBuffer(meshletCullingArgumentBuffer, RenderBackendResourceState::IndirectArgument);

                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, geometryDataBuffer);
                builder.SetBindlessResourceSRV(2, geometryInstanceDataBuffer);
                builder.SetBindlessResourceSRV(3, previousMinDepthPyramidTexture);
                builder.SetBindlessResourceUAV(4, visibleMeshletBuffer);
                builder.SetBindlessResourceUAV(5, visibleMeshletCounterBuffer);
                builder.SetBindlessResourceUAV(6, drawIndirectArgumentBuffer);
                builder.SetShaderConstantValue(7, skipOcclusionCulling);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VirtualGeometryMeshletGroupCulling);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.DispatchIndirect(
                        computeShader,
                        pushConstantValues,
                        resourceRegistry.GetRenderBackendBufferHandle(meshletCullingArgumentBuffer),
                        0);
                };
            });

        renderGraph.AddPass(
            std::format("VirtualShadowMapDepth (Graphics, {}x{})", virtualShadowMapSize, virtualShadowMapSize),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapPageTableBuffer = builder.ReadBuffer(virtualShadowMapPageTableBuffer, RenderBackendResourceState::ShaderResource);
                virtualShadowMapEntryBuffer = builder.ReadBuffer(virtualShadowMapEntryBuffer, RenderBackendResourceState::ShaderResource);
                virtualShadowMapDepthTexture = builder.WriteTexture(virtualShadowMapDepthTexture, RenderBackendResourceState::UnorderedAccess);

                builder.SetRenderArea(0, 0, virtualShadowMapSize, virtualShadowMapSize);
                builder.SetAllowUAVWrites(true);

                RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::VirtualShadowMapDepthVS);
                RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::VirtualShadowMapDepthPS);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, static_cast<float>(virtualShadowMapSize), static_cast<float>(virtualShadowMapSize));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, virtualShadowMapSize, virtualShadowMapSize);
                    commandList.SetScissors(&scissor, 1);

                    {
                        RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
                        graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
                        graphicsPipelineState.depthStencilState.depthTestEnable = false; // Disable hardware depth testing
                        graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                        graphicsPipelineState.depthStencilState.stencilTestEnable = false;

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        pushConstantValues.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                        pushConstantValues.BindBufferSRV(2, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapPageTableBuffer));
                        pushConstantValues.BindBufferSRV(3, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryDataBuffer));
                        pushConstantValues.BindBufferSRV(4, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(geometryInstanceDataBuffer));
                        pushConstantValues.BindBufferSRV(5, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(visibleMeshletBuffer));
                        pushConstantValues.BindTextureSRV(6, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapEntryBuffer));
                        pushConstantValues.BindTextureUAV(7, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(virtualShadowMapDepthTexture, 0));

                        for (uint32 shadowViewIndex = 0; shadowViewIndex < virtualShadowMapManager->GetVirtualShadowMapEntryCount(); shadowViewIndex++)
                        {
                            pushConstantValues.OverrideShaderConstantValue(8, shadowViewIndex);

                            commandList.DrawIndirect(
                                vertexShader,
                                pixelShader,
                                graphicsPipelineState,
                                pushConstantValues,
                                resourceRegistry.GetRenderBackendBufferHandle(drawIndirectArgumentBuffer),
                                0,
                                1,
                                RenderBackendPrimitiveTopology::TriangleList);
                        }
                    }

                    // @todo Remove this
                    {
                        RenderBackendViewport viewport(0.0f, 0.0f, float(renderResolution.width), float(renderResolution.height));
                        commandList.SetViewports(&viewport, 1);

                        RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                        commandList.SetScissors(&scissor, 1);
                    }
                };
            });

        intermediateResources.virtualShadowMapPageTableBuffer = virtualShadowMapPageTableBuffer;
        intermediateResources.virtualShadowMapEntryBuffer = virtualShadowMapEntryBuffer;
    }

    void RasterizationRenderer::DispatchVirtualShadowMapProjection(RenderGraph& renderGraph, const SceneView& view)
    {
        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        RenderBackendBufferHandle& virtualShadowMapShaderParameterBuffer = virtualShadowMapShaderParameterBuffers[currentPerFrameDataBufferIndex];

        RenderGraphTextureDescription screenSpaceShadowMaskTextureDesc = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R8G8B8A8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
            RenderGraphTextureHandle screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture");

        RenderGraphTextureHandle debugVisualizationTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "VirtualShadowMapDebugVisualizationTexture");

        uint32 virtualShadowMapDebugVisualizationMode = 0;
        if (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::VirtualShadowMapMipmap)
        {
            virtualShadowMapDebugVisualizationMode = 0;
        }
        if (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::VirtualShadowMapVirtualPage)
        {
            virtualShadowMapDebugVisualizationMode = 1;
        }

        renderGraph.AddPass(
            std::format("VirtualShadowMapProjection (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphBufferHandle virtualShadowMapPageTableBuffer = builder.ReadBuffer(intermediateResources.virtualShadowMapPageTableBuffer, RenderBackendResourceState::ShaderResource);
                RenderGraphBufferHandle virtualShadowMapEntryBuffer = builder.ReadBuffer(intermediateResources.virtualShadowMapEntryBuffer, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle depthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle virtualShadowMapDepthTexture = builder.ReadTexture(intermediateResources.virtualShadowMapDepthTexture, RenderBackendResourceState::ShaderResource);
                screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);
                debugVisualizationTexture = builder.WriteTexture(debugVisualizationTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    pushConstantValues.BindBufferSRV(2, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapPageTableBuffer));
                    pushConstantValues.BindBufferSRV(3, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapEntryBuffer));
                    pushConstantValues.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(depthTexture));
                    pushConstantValues.BindTextureSRV(5, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(virtualShadowMapDepthTexture));
                    pushConstantValues.BindTextureUAV(6, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture, 0));
                    pushConstantValues.BindTextureUAV(7, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(debugVisualizationTexture, 0));
                    pushConstantValues.OverrideShaderConstantValue(8, virtualShadowMapDebugVisualizationMode);

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VirtualShadowMapProjection);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        intermediateResources.shadowMaskTexture = screenSpaceShadowMaskTexture;
        intermediateResources.virtualShadowMapDebugVisualizationTexture = debugVisualizationTexture;

        const bool isVirtualShadowMapDebugVisualizationEnabled =
            (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::VirtualShadowMapMipmap) ||
            (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::VirtualShadowMapVirtualPage);
        if (isVirtualShadowMapDebugVisualizationEnabled)
        {
            debugVisualizationCallback = std::bind(&RasterizationRenderer::DispatchVirtualShadowMapDebugVisualization, this, std::placeholders::_1, std::placeholders::_2);
        }
    }

    RenderGraphTextureHandle RasterizationRenderer::DispatchVirtualShadowMapDebugVisualization(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");

        renderGraph.AddPass(
            std::format("VisualizeVirtualShadowMap (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle debugVisualizationTexture = builder.ReadTexture(intermediateResources.virtualShadowMapDebugVisualizationTexture, RenderBackendResourceState::ShaderResource);

                debugVisualizationTexture = builder.ReadTexture(debugVisualizationTexture, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(debugVisualizationTexture));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VisualizeVirtualShadowMap);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }
}