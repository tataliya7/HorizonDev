#include "RealTimeRenderer.h"
#include "Engine/Classes/Scene.h"

namespace Horizon
{
    uint32 VirtualShadowMapPageSize = 128;

    void RealTimeRenderer::DispatchVirtualShadowMapPassDrawCommands(
        RenderBackendCommandList& commandList,
        const LightRenderObject& light,
        RenderBackendBufferHandle virtualShadowMapShaderParameterBuffer,
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
            shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
            shaderConstants.BindBufferSRV(3, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
            shaderConstants.BindTextureUAV(4, renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(virtualShadowMapDepthTexture, 0));

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

    struct VirtualShadowMapShaderParameters
    {
        uint32 physicalPageCount;
        Matrix4x4f worldToClipMatrix;
    };

    void SetupVirtualShadowMapShaderParameters(VirtualShadowMapShaderParameters& outParameters, const SceneView& view, const LightRenderObject& light)
    {
        const Vector3& lightDirection = light.GetDirection();
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

            // Get frustum center
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

            glm::mat viewMatrix = glm::lookAt(frustumCenter, frustumCenter + lightDirection, glm::vec3(0.0f, 1.0f, 0.0f));

            float far = radius;
            float near = -radius;

            glm::mat projectionMatrix = glm::ortho(-radius, radius, -radius, radius, far, near);

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

        VirtualShadowMapShaderParameters virtualShadowMapShaderParameters;
        SetupVirtualShadowMapShaderParameters(virtualShadowMapShaderParameters, view, *light);

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

        const uint32 virtualShadowMapSize = 128 * 128;

        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        RenderGraphBufferDesc virtualShadowMapPhysicalPageDataBufferDesc = RenderGraphBufferDesc::CreateByteAddress(virtualShadowMapShaderParameters.physicalPageCount * sizeof(uint32));
        RenderGraphBufferHandle virtualShadowMapPhysicalPageDataBuffer = renderGraph.CreateBuffer(virtualShadowMapPhysicalPageDataBufferDesc, "VirtualShadowMapPhysicalPageDataBuffer");

        RenderGraphBufferDesc virtualShadowMapIndirectArgumentBufferDesc = RenderGraphBufferDesc::CreateIndirectArguments(sizeof(uint32), 3);
        RenderGraphBufferHandle virtualShadowMapIndirectArgumentBuffer = renderGraph.CreateBuffer(virtualShadowMapIndirectArgumentBufferDesc, "VirtualShadowMapIndirectArgumentBuffer");

        RenderGraphBufferDesc virtualShadowMapActivePhysicalPageIndexBufferDesc = RenderGraphBufferDesc::CreateByteAddress(virtualShadowMapShaderParameters.physicalPageCount * sizeof(uint32));
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

#if 1
        renderGraph.AddPass(
            std::format("VirtualShadowMapClearPageDataBuffer ({} bytes)", virtualShadowMapPhysicalPageDataBufferDesc.size),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapPhysicalPageDataBuffer = builder.WriteBuffer(virtualShadowMapPhysicalPageDataBuffer, RenderBackendResourceState::UnorderedAccess);
                //virtualShadowMapPhysicalPageDataBuffer = builder.WriteBuffer(virtualShadowMapPhysicalPageDataBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.ClearBufferUAV(registry.GetRenderBackendBufferHandle(virtualShadowMapPhysicalPageDataBuffer), 0);
                };
            });

        renderGraph.AddPass(
            std::format("VirtualShadowMapPageRequest (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                virtualShadowMapPhysicalPageDataBuffer = builder.WriteBuffer(virtualShadowMapPhysicalPageDataBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = CeilDiv(renderResolution.width, 8);
                    uint32 threadGroupCountY = CeilDiv(renderResolution.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindBufferUAV(3, registry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapPhysicalPageDataBuffer));

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
            std::format("VirtualShadowMapClearIndirectArgumentBuffer (Compute, 1x1x1)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapIndirectArgumentBuffer = builder.WriteBuffer(virtualShadowMapIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = 1;
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferUAV(0, registry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapIndirectArgumentBuffer));

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
            std::format("VirtualShadowMapPhysicalMemoryAllocation (Compute, {}x1x1)", virtualShadowMapShaderParameters.physicalPageCount),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                virtualShadowMapPhysicalPageDataBuffer = builder.ReadBuffer(virtualShadowMapPhysicalPageDataBuffer, RenderBackendResourceState::ShaderResource);
                virtualShadowMapIndirectArgumentBuffer = builder.WriteBuffer(virtualShadowMapIndirectArgumentBuffer, RenderBackendResourceState::UnorderedAccess);
                virtualShadowMapActivePhysicalPageIndexBuffer = builder.WriteBuffer(virtualShadowMapActivePhysicalPageIndexBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = CeilDiv(virtualShadowMapShaderParameters.physicalPageCount, 64);
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    shaderConstants.BindBufferSRV(1, registry.GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapPhysicalPageDataBuffer));
                    shaderConstants.BindBufferUAV(2, registry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapIndirectArgumentBuffer));
                    shaderConstants.BindBufferUAV(3, registry.GetBufferUAVBindlessResourceDescriptorIndex(virtualShadowMapActivePhysicalPageIndexBuffer));
                    shaderConstants.BindScalar(4, 0);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VirtualShadowMapPhysicalMemoryAllocation);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

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
                    //shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
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
               virtualShadowMapDepthTexture = builder.WriteTexture(virtualShadowMapDepthTexture, RenderBackendResourceState::UnorderedAccess);

               builder.SetRenderArea(0, 0, virtualShadowMapSize, virtualShadowMapSize);

               return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
               {
                   RenderBackendViewport viewport(0.0f, 0.0f, float(virtualShadowMapSize), float(virtualShadowMapSize));
                   commandList.SetViewports(&viewport, 1);

                   RenderBackendScissor scissor(0, 0, virtualShadowMapSize, virtualShadowMapSize);
                   commandList.SetScissors(&scissor, 1);

                   DispatchVirtualShadowMapPassDrawCommands(commandList, *light, virtualShadowMapShaderParameterBuffer, registry.GetRenderBackendTextureHandle(virtualShadowMapDepthTexture));

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
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
            RenderGraphTextureHandle screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture");

        renderGraph.AddPass(
            std::format("VirtualShadowMapProjection (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle virtualShadowMapDepthTexture = builder.ReadTexture(sceneTextures.virtualShadowMapDepthTexture, RenderBackendResourceState::ShaderResource);
                screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = CeilDiv(renderResolution.width, 8);
                    uint32 threadGroupCountY = CeilDiv(renderResolution.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(virtualShadowMapShaderParameterBuffer));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(virtualShadowMapDepthTexture));
                    shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndex(screenSpaceShadowMaskTexture, 0));

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
    }
}