#include "VirtualShadowMaps.h"
#include "RealTimeRenderer.h"

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

    void RealTimeRenderer::DispatchVirtualShadowMapPassDrawCommands(
        RenderBackendCommandList& commandList,
        const LightRenderObject& light,
        RenderBackendBufferHandle virtualShadowMapShaderParameterBuffer,
        RenderBackendBufferHandle virtualShadowMapPageTableBuffer,
        RenderBackendTextureHandle virtualShadowMapDepthTexture)
    {
        const GeometryPassDrawCommandList& drawCommandList = geometryPassDrawCommandLists[uint32(GeometryPassType::VirtualShadowMap)];
        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();//drawCommandList.setupJobData.scene->GetGPUScene();

        RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::VirtualShadowMapDepthVS);
        RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::VirtualShadowMapDepthPS);

        for (uint32 drawCommandIndex = 0; drawCommandIndex < drawCommandList.drawCommandCount; drawCommandIndex++)
        {
            const GeometryPassDrawCommand& drawCommand = drawCommandList.commands[drawCommandIndex];

            RenderBackendGraphicsPipelineState graphicsPipelineState = {};
            graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
            graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
            // Disable hardware depth testing
            graphicsPipelineState.depthStencilState.depthTestEnable = false;
            graphicsPipelineState.depthStencilState.depthWriteEnable = false;
            graphicsPipelineState.depthStencilState.stencilTestEnable = false;

            RenderBackendShaderConstants shaderConstants = {};
            shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
            shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
            shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapPageTableBuffer));
            shaderConstants.BindBufferSRV(3, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
            shaderConstants.BindBufferSRV(4, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
            shaderConstants.BindTextureUAV(5, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(virtualShadowMapDepthTexture, 0));
            shaderConstants.BindTextureSRV(6, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapEntryBuffers[currentPerFrameDataBufferIndex]));

            for (uint32 shadowViewIndex = 0; shadowViewIndex < virtualShadowMapManager->GetVirtualShadowMapEntryCount(); shadowViewIndex++)
            {
                shaderConstants.BindScalar(7, shadowViewIndex);

                commandList.DrawIndexed(
                    vertexShader,
                    pixelShader,
                    graphicsPipelineState,
                    shaderConstants,
                    drawCommand.indexBuffer,
                    drawCommand.indexCount,
                    drawCommand.instanceCount,
                    drawCommand.firstIndex,
                    0, // TODO
                    drawCommand.firstInstance,
                    drawCommand.topology);
            }
        }
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

    void RealTimeRenderer::RenderVirtualShadowMapDepth(RenderGraph& renderGraph, const SceneView& view)
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
            RenderBackendBufferDesc virtualShadowMapShaderParameterUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(sizeof(VirtualShadowMapShaderParameters));
            virtualShadowMapShaderParameterUploadBuffer = renderBackend->CreateBuffer(&virtualShadowMapShaderParameterUploadBufferDesc, nullptr, "VirtualShadowMapShaderParameterUploadBuffer");
            RenderBackendBufferDesc virtualShadowMapShaderParameterBufferDesc = RenderBackendBufferDesc::CreateStructured(sizeof(VirtualShadowMapShaderParameters), 1);
            virtualShadowMapShaderParameterBuffer = renderBackend->CreateBuffer(&virtualShadowMapShaderParameterBufferDesc, nullptr, "VirtualShadowMapShaderParameterBuffer");
        }
        renderBackend->UpdateBuffer(virtualShadowMapShaderParameterUploadBuffer, 0, &virtualShadowMapShaderParameters, sizeof(VirtualShadowMapShaderParameters));

        renderGraph.AddPass(
            std::format("UpdateVirtualShadowMapShaderParameterBuffer (Copy, {} bytes)", sizeof(VirtualShadowMapShaderParameters)),
            RenderGraphPassFlags::Copy,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
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
        RenderBackendBufferHandle& virtualShadowMapEntryUploadBuffer = virtualShadowMapEntryUploadBuffers[currentPerFrameDataBufferIndex];
        RenderBackendBufferHandle& virtualShadowMapEntryBuffer = virtualShadowMapEntryBuffers[currentPerFrameDataBufferIndex];
        if (!virtualShadowMapEntryBuffer)
        {
            RenderBackendBufferDesc virtualShadowMapEntryUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(virtualShadowMapEntryBufferSize);
            virtualShadowMapEntryUploadBuffer = renderBackend->CreateBuffer(&virtualShadowMapEntryUploadBufferDesc, nullptr, "VirtualShadowMapEntryUploadBuffer");
            RenderBackendBufferDesc virtualShadowMapEntryBufferDesc = RenderBackendBufferDesc::CreateByteAddress(virtualShadowMapEntryBufferSize);
            virtualShadowMapEntryBuffer = renderBackend->CreateBuffer(&virtualShadowMapEntryBufferDesc, nullptr, "VirtualShadowMapEntryBuffer");
        }
        renderBackend->UpdateBuffer(virtualShadowMapEntryUploadBuffer, 0, virtualShadowMapManager->virtualShadowMapEntries.data(), virtualShadowMapEntryBufferSize);

        renderGraph.AddPass(
            std::format("UpdateVirtualShadowMapEntryBuffer (Copy, {} bytes)", virtualShadowMapEntryBufferSize),
            RenderGraphPassFlags::Copy,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    // RenderBackendBarrier barrierBefore[] =
                    // {
                    //     RenderBackendBarrier(virtualShadowMapEntryBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
                    // };
                    // commandList.Barriers(barrierBefore, 1);

                    commandList.CopyBuffer(
                        virtualShadowMapEntryUploadBuffer,
                        0,
                        virtualShadowMapEntryBuffer,
                        0,
                        virtualShadowMapEntryBufferSize);

                    RenderBackendBarrier barrierAfter[] =
                    {
                        RenderBackendBarrier(virtualShadowMapEntryBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::ShaderResource)
                    };
                    commandList.Barriers(barrierAfter, 1);
                };
            });

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

        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        RenderGraphBufferDesc virtualShadowMapPageRequestBufferDesc = RenderGraphBufferDesc::CreateByteAddress(virtualShadowMapMaximumVirtualPageCount * sizeof(uint32));
        RenderGraphBufferHandle virtualShadowMapPageRequestBuffer = renderGraph.CreateBuffer(virtualShadowMapPageRequestBufferDesc, "VirtualShadowMapPageRequestBuffer");

        RenderGraphBufferDesc virtualShadowMapPageTableBufferDesc = RenderGraphBufferDesc::CreateByteAddress(virtualShadowMapMaximumVirtualPageCount * sizeof(uint32));
        RenderGraphBufferHandle virtualShadowMapPageTableBuffer = renderGraph.CreateBuffer(virtualShadowMapPageTableBufferDesc, "VirtualShadowMapPageTableBuffer");

        RenderGraphBufferDesc virtualShadowMapPhysicalPageDataBufferDesc = RenderGraphBufferDesc::CreateByteAddress(virtualShadowMapPhysicalPageCount * sizeof(uint32));
        RenderGraphBufferHandle virtualShadowMapPhysicalPageDataBuffer = renderGraph.CreateBuffer(virtualShadowMapPhysicalPageDataBufferDesc, "VirtualShadowMapPhysicalPageDataBuffer");

        RenderGraphBufferDesc virtualShadowMapPhysicalPageListBufferDesc = RenderGraphBufferDesc::CreateByteAddress(sizeof(uint32), 1);
        RenderGraphBufferHandle virtualShadowMapPhysicalPageListBuffer = renderGraph.CreateBuffer(virtualShadowMapPhysicalPageListBufferDesc, "VirtualShadowMapPhysicalPageListBuffer");

        RenderGraphBufferDesc virtualShadowMapIndirectArgumentBufferDesc = RenderGraphBufferDesc::CreateIndirectArguments(sizeof(uint32), 3);
        RenderGraphBufferHandle virtualShadowMapIndirectArgumentBuffer = renderGraph.CreateBuffer(virtualShadowMapIndirectArgumentBufferDesc, "VirtualShadowMapIndirectArgumentBuffer");

        RenderGraphBufferDesc virtualShadowMapActivePhysicalPageIndexBufferDesc = RenderGraphBufferDesc::CreateByteAddress(virtualShadowMapPhysicalPageCount * sizeof(uint32));
        RenderGraphBufferHandle virtualShadowMapActivePhysicalPageIndexBuffer = renderGraph.CreateBuffer(virtualShadowMapActivePhysicalPageIndexBufferDesc, "VirtualShadowMapActivePhysicalPageIndexBuffer");

        RenderGraphTextureDesc virtualShadowMapDepthTextureDesc = RenderGraphTextureDesc::Create2DArray(
            virtualShadowMapSize,
            virtualShadowMapSize,
            RenderBackendTextureFormat::R32Uint,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            1,
            RenderBackendTextureClearValue::Black,
            1,
            1,
            RenderBackendResourceState::UnorderedAccess);

        RenderGraphTextureHandle virtualShadowMapDepthTexture = sceneTextures.virtualShadowMapDepthTexture = renderGraph.CreateTexture(virtualShadowMapDepthTextureDesc, "VirtualShadowMapDepthTexture");

        renderGraph.AddPass(
            std::format("VirtualShadowMapClearPageDataBuffer ({} bytes)", virtualShadowMapPhysicalPageDataBufferDesc.size),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapPageRequestBuffer = builder.WriteBuffer(virtualShadowMapPageRequestBuffer, RenderBackendResourceState::UnorderedAccess);
                virtualShadowMapPhysicalPageDataBuffer = builder.WriteBuffer(virtualShadowMapPhysicalPageDataBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.ClearBufferUAV(registry.GetRenderBackendBufferHandle(virtualShadowMapPageRequestBuffer), 0);
                    commandList.ClearBufferUAV(registry.GetRenderBackendBufferHandle(virtualShadowMapPhysicalPageDataBuffer), 0);
                };
            });

        renderGraph.AddPass(
            std::format("VirtualShadowMapClearIndirectArgumentBuffer (Compute, 1x1x1)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapIndirectArgumentBuffer = builder.WriteBuffer(virtualShadowMapIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);
                virtualShadowMapPhysicalPageListBuffer = builder.WriteBuffer(virtualShadowMapPhysicalPageListBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = 1;
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferUAV(0, registry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapIndirectArgumentBuffer));
                    shaderConstants.BindBufferUAV(1, registry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapPhysicalPageListBuffer));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VirtualShadowMapClearIndirectArgumentBuffer);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
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

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(virtualShadowMapMaximumVirtualPageCount, 64);
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    shaderConstants.BindBufferUAV(1, registry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapPageTableBuffer));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VirtualShadowMapClearPageTable);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
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
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                virtualShadowMapPageRequestBuffer = builder.WriteBuffer(virtualShadowMapPageRequestBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindBufferUAV(3, registry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapPageRequestBuffer));
                    shaderConstants.BindBufferSRV(4, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapEntryBuffer));
                    shaderConstants.BindScalar(5, distantLightVirtualShadowMapEntryCount);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VirtualShadowMapPageRequest);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
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

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(virtualShadowMapMaximumVirtualPageCount, 64);
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapPageRequestBuffer));
                    shaderConstants.BindBufferUAV(2, registry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapPhysicalPageListBuffer));
                    shaderConstants.BindBufferUAV(3, registry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapPageTableBuffer));
                    shaderConstants.BindBufferUAV(4, registry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapPhysicalPageDataBuffer));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VirtualShadowMapPhysicalPageAllocation);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        sceneTextures.virtualShadowMapPageTableBuffer = virtualShadowMapPageTableBuffer;

        renderGraph.AddPass(
            std::format("VirtualShadowMapPhysicalMemoryAllocation (Compute, {}x1x1)", virtualShadowMapPhysicalPageCount),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapPhysicalPageDataBuffer = builder.ReadBuffer(virtualShadowMapPhysicalPageDataBuffer, RenderBackendResourceState::ShaderResource);
                virtualShadowMapIndirectArgumentBuffer = builder.WriteBuffer(virtualShadowMapIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);
                virtualShadowMapActivePhysicalPageIndexBuffer = builder.WriteBuffer(virtualShadowMapActivePhysicalPageIndexBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(virtualShadowMapPhysicalPageCount, 64);
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapPhysicalPageDataBuffer));
                    shaderConstants.BindBufferUAV(2, registry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapIndirectArgumentBuffer));
                    shaderConstants.BindBufferUAV(3, registry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapActivePhysicalPageIndexBuffer));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VirtualShadowMapPhysicalMemoryAllocation);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
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
                virtualShadowMapDepthTexture = builder.WriteTexture(sceneTextures.virtualShadowMapDepthTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapActivePhysicalPageIndexBuffer));
                    shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(virtualShadowMapDepthTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VirtualShadowMapClearPhysicalMemory);

                    commandList.DispatchIndirect(
                        computeShader,
                        shaderConstants,
                        registry.GetRenderBackendBufferHandle(virtualShadowMapIndirectArgumentBuffer),
                        0);
                };
            });
#else
        renderGraph.AddPass(
            std::format("VirtualShadowMapClearPhysicalMemory (Compute, Indirect)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapDepthTexture = builder.WriteTexture(sceneTextures.virtualShadowMapDepthTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.ClearTextureUAV(
                        RenderBackendTextureUAVDesc(registry.GetRenderBackendTextureHandle(virtualShadowMapDepthTexture), 0),
                        RenderBackendTextureClearValue::Black);
                };
            });
#endif

        renderGraph.AddPass(
           std::format("VirtualShadowMapDepth (Graphics, {}x{})", virtualShadowMapSize, virtualShadowMapSize),
           RenderGraphPassFlags::Graphics,
           [&](RenderGraphBuilder& builder)
           {
               virtualShadowMapPageTableBuffer = builder.ReadBuffer(virtualShadowMapPageTableBuffer, RenderBackendResourceState::ShaderResource);
               virtualShadowMapDepthTexture = builder.WriteTexture(virtualShadowMapDepthTexture, RenderBackendResourceState::UnorderedAccess);

               builder.SetRenderArea(0, 0, virtualShadowMapSize, virtualShadowMapSize);
               builder.SetAllowUAVWrites(true);

               return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
               {
                   RenderBackendViewport viewport(0.0f, 0.0f, float(virtualShadowMapSize), float(virtualShadowMapSize));
                   commandList.SetViewports(&viewport, 1);

                   RenderBackendScissor scissor(0, 0, virtualShadowMapSize, virtualShadowMapSize);
                   commandList.SetScissors(&scissor, 1);

                   DispatchVirtualShadowMapPassDrawCommands(
                       commandList,
                       *light,
                       virtualShadowMapShaderParameterBuffer,
                       registry.GetRenderBackendBufferHandle(virtualShadowMapPageTableBuffer),
                       registry.GetRenderBackendTextureHandle(virtualShadowMapDepthTexture));

                   {
                       RenderBackendViewport viewport(0.0f, 0.0f, float(renderResolution.width), float(renderResolution.height));
                       commandList.SetViewports(&viewport, 1);

                       RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                       commandList.SetScissors(&scissor, 1);
                   }
               };
           });
    }

    void RealTimeRenderer::DispatchVirtualShadowMapProjection(RenderGraph& renderGraph, const SceneView& view)
    {
        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        RenderBackendBufferHandle& virtualShadowMapShaderParameterBuffer = virtualShadowMapShaderParameterBuffers[currentPerFrameDataBufferIndex];

        RenderGraphTextureDesc screenSpaceShadowMaskTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R8G8B8A8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
            RenderGraphTextureHandle screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture");

        RenderGraphTextureHandle debugVisualizationTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "VirtualShadowMapDebugVisualizationTexture");

        uint32 virtualShadowMapDebugVisualizationMode = 0;
        if (view.debugVisualizationMode == SceneViewDebugVisualizationMode::VirtualShadowMapMipmap)
        {
            virtualShadowMapDebugVisualizationMode = 0;
        }
        if (view.debugVisualizationMode == SceneViewDebugVisualizationMode::VirtualShadowMapVirtualPage)
        {
            virtualShadowMapDebugVisualizationMode = 1;
        }

        renderGraph.AddPass(
            std::format("VirtualShadowMapProjection (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphBufferHandle virtualShadowMapPageTableBuffer = builder.ReadBuffer(sceneTextures.virtualShadowMapPageTableBuffer, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle virtualShadowMapDepthTexture = builder.ReadTexture(sceneTextures.virtualShadowMapDepthTexture, RenderBackendResourceState::ShaderResource);
                screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);
                debugVisualizationTexture = builder.WriteTexture(debugVisualizationTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    shaderConstants.BindBufferSRV(2, registry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapPageTableBuffer));
                    shaderConstants.BindBufferSRV(3, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapEntryBuffers[currentPerFrameDataBufferIndex]));
                    shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindTextureSRV(5, registry.GetTextureSRVBindlessResourceDescriptorIndex(virtualShadowMapDepthTexture));
                    shaderConstants.BindTextureUAV(6, registry.GetTextureUAVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture, 0));
                    shaderConstants.BindTextureUAV(7, registry.GetTextureUAVBindlessResourceDescriptorIndex(debugVisualizationTexture, 0));
                    shaderConstants.BindScalar(8, virtualShadowMapDebugVisualizationMode);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VirtualShadowMapProjection);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        sceneTextures.shadowMaskTexture = screenSpaceShadowMaskTexture;
        sceneTextures.virtualShadowMapDebugVisualizationTexture = debugVisualizationTexture;
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
}