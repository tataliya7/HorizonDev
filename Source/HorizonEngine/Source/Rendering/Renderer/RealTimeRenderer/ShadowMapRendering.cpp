#include "RealTimeRenderer.h"

namespace Horizon
{
    static void SetupCascadedShadowMapShaderParameters(CascadedShadowMapShaderParameters& outCascades, const SceneView& view, const LightRenderObject& light)
    {
        for (uint32 i = 0; i < RendererMaxShadowMapCascadeCount; i++)
        {
            outCascades.viewProjectionMatrix[i] = Matrix4x4f(1.0f);
            outCascades.cascadeEndDistance[i] = std::numeric_limits<float>::max();
            outCascades.transitionStartDistance[i] = std::numeric_limits<float>::max();
        }
        outCascades.shadowFadeoutParameters = Vector2(0.0f, 1.0f);
        outCascades.useTransition = false;
        outCascades.cascadeCount = 0;

        const Vector3& lightDirection = light.GetDirection();
        const uint32 shadowCascadeCount = light.GetShadowCascadeCount();
        const float maxShadowDistance = light.GetMaxShadowDistance();

        //Matrix4x4f worldToLightMatrix = light.color;

        float cascadeSplits[RendererMaxShadowMapCascadeCount];

        float nearClip = view.nearClippingPlane;
        float farClip = maxShadowDistance;//camera.farClippingPlane;
        float fieldOfView = view.fieldOfViewAngleVertical;
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
            glm::mat4 cameraProjectionMatrix = Math::PerspectiveReverseZ_RH_ZO(fieldOfView, aspectRatio, nearClip, farClip);
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
            radius = std::ceil(radius * 16.0f) / 16.0f;

            glm::vec3 maxExtents = glm::vec3(radius);
            glm::vec3 minExtents = -maxExtents;

            glm::mat viewMatrix = glm::lookAt(frustumCenter - lightDirection * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat projectionMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, maxExtents.z - minExtents.z, 0.0f);

            lastSplitDist = cascadeSplits[cascadeIndex];

            float cascadeEndDistance = nearClip + splitDist * clipRange;
            float transitionRange = cascadeIndex > 0 ? cascadeEndDistance - outCascades.cascadeEndDistance[cascadeIndex - 1] : splitDist * clipRange;
            transitionRange *= 0.2f;

            outCascades.viewProjectionMatrix[cascadeIndex] = projectionMatrix * viewMatrix;
            outCascades.cascadeEndDistance[cascadeIndex] = cascadeEndDistance;
            outCascades.transitionStartDistance[cascadeIndex] = cascadeEndDistance - transitionRange;
            outCascades.inverseTransitionRange[cascadeIndex] = 1.0f / transitionRange;
        }

        float shadowFadeoutRange = clipRange * light.shadowFadeoutFactor;
        float shadowFadeoutStartDistance = maxShadowDistance - shadowFadeoutRange;

        outCascades.shadowMapSize = Vector2(float(light.shadowMapSize), 1.0f / float(light.shadowMapSize));
        outCascades.shadowFadeoutParameters = Vector2(shadowFadeoutStartDistance, 1.0f / shadowFadeoutRange);
        outCascades.useTransition = 1;
        outCascades.cascadeCount = shadowCascadeCount;
    }

    void RealTimeRenderer::DispatchCascadedShadowMapPassDrawCommands(RenderBackendCommandList& commandList, const LightRenderObject& light, uint32 cascadeIndex, RenderBackendBufferHandle cascadeShadowMapDataBuffer)
    {
        const GeometryPassDrawCommandList& drawCommandList = geometryPassDrawCommandLists[uint32(GeometryPassType::CascadedShadowMap)];
        const GPUScene* gpuScene = sceneView->scene->GetGPUScene();//drawCommandList.setupJobData.scene->GetGPUScene();

        RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::CascadedShadowMapVS);
        RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::CascadedShadowMapPS);

        for (uint32 drawCommandIndex = 0; drawCommandIndex < drawCommandList.drawCommandCount; drawCommandIndex++)
        {
            const GeometryPassDrawCommand& drawCommand = drawCommandList.commands[drawCommandIndex];

            RenderBackendGraphicsPipelineState graphicsPipelineState = {};
            graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
            graphicsPipelineState.rasterizationState.fillMode = RenderBackendRasterizationFillMode::Solid;
            graphicsPipelineState.depthStencilState.depthTestEnable = true;
            graphicsPipelineState.depthStencilState.depthWriteEnable = true;
            // TODO: vkCmdSetDepthBias
            graphicsPipelineState.rasterizationState.depthBiasConstantFactor = light.shadowMapDepthBiasConstantFactor;
            graphicsPipelineState.rasterizationState.depthBiasSlopeFactor = light.shadowMapDepthBiasSlopeFactor;
            graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

            RenderBackendShaderConstants shaderConstants = {};
            shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
            shaderConstants.BindBufferCBV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(cascadeShadowMapDataBuffer));
            shaderConstants.BindBufferSRV(2, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryDataBuffer));
            shaderConstants.BindBufferSRV(3, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(gpuScene->geometryInstanceDataBuffer));
            shaderConstants.BindScalar(4, cascadeIndex);

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

    void RealTimeRenderer::RenderScreenSpaceShadows(
        RenderGraph& renderGraph,
        const SceneView& view,
        const LightRenderObject& light,
        RenderGraphTextureHandle& screenSpaceShadowMaskTexture)
    {
        CascadedShadowMapShaderParameters cascadedShadowMapShaderParameters;
        SetupCascadedShadowMapShaderParameters(cascadedShadowMapShaderParameters, view, light);

        RenderBackendBufferHandle& cascadedShadowMapDataUploadBuffer = cascadedShadowMapDataUploadBuffers[currentPerFrameDataBufferIndex];
        RenderBackendBufferHandle& cascadedShadowMapDataBuffer = cascadedShadowMapDataBuffers[currentPerFrameDataBufferIndex];
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

        RenderGraphTextureDesc shadowMapTextureDesc = RenderGraphTextureDesc::Create2DArray(
            shadowMapSize,
            shadowMapSize,
            RenderBackendTextureFormat::D32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::DepthStencil,
            shadowCascadeCount,
            RenderBackendTextureClearValue::CreateDepthValue(FarClipPlaneDepthValue));

        RenderGraphTextureHandle shadowMapTexture = renderGraph.CreateTexture(shadowMapTextureDesc, "CascadedShadowMap");

        static bool renderSM = true;
        if (renderSM)
        {
            for (uint32 cascadeIndex = 0; cascadeIndex < shadowCascadeCount; cascadeIndex++)
            {
                renderGraph.AddPass(
                    std::format("CascadedShadowMap (Graphics, {}x{}, cascade={})", shadowMapSize, shadowMapSize, cascadeIndex),
                    RenderGraphPassFlags::Graphics,
                    [&](RenderGraphBuilder& builder)
                    {
                        shadowMapTexture = builder.WriteTexture(shadowMapTexture, RenderBackendResourceState::DepthStencil);

                        builder.BindDepthStencil(shadowMapTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve, 0, cascadeIndex);

                        return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                        {
                            {
                                RenderBackendViewport viewport(0.0f, 0.0f, float(shadowMapSize), float(shadowMapSize));
                                commandList.SetViewports(&viewport, 1);

                                RenderBackendScissor scissor(0, 0, shadowMapSize, shadowMapSize);
                                commandList.SetScissors(&scissor, 1);
                            }

                            DispatchCascadedShadowMapPassDrawCommands(commandList, light, cascadeIndex, cascadedShadowMapDataBuffer);

                            {
                                RenderBackendViewport viewport(0.0f, 0.0f, float(renderResolution.width), float(renderResolution.height));
                                commandList.SetViewports(&viewport, 1);

                                RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                                commandList.SetScissors(&scissor, 1);
                            }
                        };
                    });
            }
        }

        renderGraph.AddPass(
            std::format("ShadowMapProjection (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                shadowMapTexture = builder.ReadTexture(sceneTextures.shadowMapTexture, RenderBackendResourceState::ShaderResource);
                screenSpaceShadowMaskTexture = builder.WriteTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = CeilDiv(renderResolution.width, 8);
                    uint32 threadGroupCountY = CeilDiv(renderResolution.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(cascadedShadowMapDataBuffer));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(shadowMapTexture));
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

        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        sceneTextures.cascadedShadowMapDataBuffer = cascadedShadowMapDataBuffer;
        sceneTextures.shadowMapTexture = shadowMapTexture;
        sceneTextures.shadowMaskTexture = screenSpaceShadowMaskTexture;
    }
}