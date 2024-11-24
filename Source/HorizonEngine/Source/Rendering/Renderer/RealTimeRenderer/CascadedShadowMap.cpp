#include "RealTimeRenderer.h"
#include "ShadowMapping.h"

namespace Horizon
{
    /**
     * Zhang, Fan, Hanqiu Sun, Leilei Xu, and Kit-Lun Lee. 2006. "Parallel-Split Shadow Maps for Large-Scale Virtual Environments."
     * In Proceedings of ACM International Conference on Virtual Reality Continuum and Its Applications 2006, pp. 311–318.
     */
    float CascadedShadowMapPracticalSplitScheme(uint32 cascadeIndex, uint32 cascadeCount, float nearPlane, float farPlane, float lambda)
    {
        if (cascadeIndex == 0)
        {
            return nearPlane;
        }

        if (cascadeIndex == cascadeCount)
        {
            return farPlane;
        }

        float t = float(cascadeIndex) / float(cascadeCount);
        float logarithmicSplitScheme = nearPlane * std::pow((farPlane / nearPlane), t);
        float uniformSplitScheme = nearPlane + (farPlane - nearPlane) * t;

        return lambda * logarithmicSplitScheme + (1.0f - lambda) * uniformSplitScheme;
    }

    /**
     * Calculate minimal bounding sphere of frustum.
     */
    Vector4f ComputeViewSpaceShadowCascadeMinimumBoundingSphere(float n, float f, float tanHalfVerticalFOV, float aspectRatio)
    {
        float k = tanHalfVerticalFOV * std::sqrt(1.0f + aspectRatio * aspectRatio);

        float boundingSphereCenterDistance = f;
        float boundingSphereRadius = f * k;

        if ((k * k) < ((f - n) / (f + n)))
        {
            boundingSphereCenterDistance = 0.5f * (f + n) * (1.0f + k * k);
            boundingSphereRadius = 0.5f * std::sqrt(
                (f - n) * (f - n) +
                2.0f * k * k * (f * f + n * n) +
                (f + n) * (f + n) * k * k * k * k);
        }

        return Vector4f(0.0f, 0.0f, -boundingSphereCenterDistance, boundingSphereRadius);
    }

    void InitializeCascadedShadowMapShaderParameters(CascadedShadowMapShaderParameters& outParameters)
    {
        for (uint32 i = 0; i < RendererMaxShadowMapCascadeCount; i++)
        {
            outParameters.worldToClipMatrix[i] = Matrix4x4f(1.0f);
            outParameters.cascadeEndDistance[i] = std::numeric_limits<float>::max();
            outParameters.transitionStartDistance[i] = std::numeric_limits<float>::max();
            outParameters.inverseTransitionRange[i] = 0.0f;
        }
        outParameters.shadowFadeOutParameters = Vector2f(0.0f, 1.0f);
        outParameters.maxShadowDistance = 0.0f;
        outParameters.cascadeCount = 0;
    }

    void SetupCascadedShadowMapShaderParameters(CascadedShadowMapShaderParameters& outParameters, const SceneView& view, const LightRenderObject& light, const CascadedShadowMapRenderData& data)
    {
        InitializeCascadedShadowMapShaderParameters(outParameters);

        for (uint32 cascadeIndex = 0; cascadeIndex < data.cascadeCount; cascadeIndex++)
        {
            outParameters.worldToClipMatrix[cascadeIndex] = data.cascadeData[cascadeIndex].worldToClipMatrix;
            outParameters.cascadeEndDistance[cascadeIndex] = data.cascadeData[cascadeIndex].endDistance;
            outParameters.transitionStartDistance[cascadeIndex] = data.cascadeData[cascadeIndex].endDistance - data.cascadeData[cascadeIndex].transitionRange;
            outParameters.inverseTransitionRange[cascadeIndex] = 1.0f / std::max(data.cascadeData[cascadeIndex].transitionRange, 0.0001f);
        }

        float shadowRange = light.maxShadowDistance - view.nearClippingPlane;
        float shadowFadeOutRange = shadowRange * light.shadowFadeOutFactor;
        float shadowFadeOutStartDistance = light.maxShadowDistance - shadowFadeOutRange;

        outParameters.shadowMapSize = Vector2f(float(data.resolution), 1.0f / float(data.resolution));
        outParameters.shadowFadeOutParameters = Vector2f(shadowFadeOutStartDistance, 1.0f / std::max(shadowFadeOutRange, 0.0001f));
        outParameters.maxShadowDistance = light.maxShadowDistance;
        outParameters.cascadeCount = data.cascadeCount;
    }

    void SetupViewDependentCascadedShadowMapRenderDataForLight(CascadedShadowMapRenderData& outCascadedShadowMapRenderData, const SceneView& view, const LightRenderObject& light)
    {
        const Matrix4x4f& inverseViewMatrix = view.transformations.viewToWorldMatrix;
        const float cameraNearClippingPlane = view.nearClippingPlane;
        const float cameraFarClippingPlane = view.farClippingPlane;
        const float tanHalfVerticalFOV = view.tanHalfVerticalFOV;
        const float aspectRatio = view.aspectRatio;
        const uint32 shadowMapSize = light.shadowMapSize;
        const uint32 shadowCascadeCount = light.shadowCascadeCount;
        const float shadowCascadeSplitLambda = light.shadowCascadeSplitLambda;
        const float shadowCascadeTransitionScale = light.shadowCascadeTransitionScale;
        const Vector3f lightDirection = light.GetDirection();
        const float maxShadowDistance = std::min(light.maxShadowDistance, cameraFarClippingPlane);

        outCascadedShadowMapRenderData.resolution = shadowMapSize;
        outCascadedShadowMapRenderData.cascadeCount = shadowCascadeCount;

        for (uint32 cascadeIndex = 0; cascadeIndex < shadowCascadeCount; cascadeIndex++)
        {
            ShadowMapCascadeData& cascadeData = outCascadedShadowMapRenderData.cascadeData[cascadeIndex];

            float cascadeStartDistance = CascadedShadowMapPracticalSplitScheme(cascadeIndex, shadowCascadeCount, cameraNearClippingPlane, maxShadowDistance, shadowCascadeSplitLambda);
            float cascadeEndDistance = CascadedShadowMapPracticalSplitScheme(cascadeIndex + 1, shadowCascadeCount, cameraNearClippingPlane, maxShadowDistance, shadowCascadeSplitLambda);

            float transitionRange = (cascadeEndDistance - cascadeStartDistance) * shadowCascadeTransitionScale;

#if 0
            float halfCascadeFrustumNearPlaneExtentX = cascadeStartDistance * tanHalfVerticalFOV * aspectRatio;
            float halfCascadeFrustumNearPlaneExtentY = cascadeStartDistance * tanHalfVerticalFOV;

            float halfCascadeFrustumFarPlaneExtentX = cascadeEndDistance * tanHalfVerticalFOV * aspectRatio;
            float halfCascadeFrustumFarPlaneExtentY = cascadeEndDistance * tanHalfVerticalFOV;

            Vector4f cascadeFrustumCorners[8] =
            {
                Vector4f( halfCascadeFrustumNearPlaneExtentX,  halfCascadeFrustumNearPlaneExtentY, -cascadeStartDistance, 1.0f), // top right
                Vector4f( halfCascadeFrustumNearPlaneExtentX, -halfCascadeFrustumNearPlaneExtentY, -cascadeStartDistance, 1.0f), // bottom right
                Vector4f(-halfCascadeFrustumNearPlaneExtentX,  halfCascadeFrustumNearPlaneExtentY, -cascadeStartDistance, 1.0f), // top left
                Vector4f(-halfCascadeFrustumNearPlaneExtentX, -halfCascadeFrustumNearPlaneExtentY, -cascadeStartDistance, 1.0f), // bottom left
                Vector4f( halfCascadeFrustumFarPlaneExtentX,  halfCascadeFrustumFarPlaneExtentY, -cascadeEndDistance, 1.0f), // top right
                Vector4f( halfCascadeFrustumFarPlaneExtentX, -halfCascadeFrustumFarPlaneExtentY, -cascadeEndDistance, 1.0f), // bottom right
                Vector4f(-halfCascadeFrustumFarPlaneExtentX,  halfCascadeFrustumFarPlaneExtentY, -cascadeEndDistance, 1.0f), // top left
                Vector4f(-halfCascadeFrustumFarPlaneExtentX, -halfCascadeFrustumFarPlaneExtentY, -cascadeEndDistance, 1.0f)  // bottom left
            };

            Vector3f boundingSphereCenter = Vector3f(0.0f, 0.0f, 0.0f);
            for (uint32 i = 0; i < 8; i++)
            {
                cascadeFrustumCorners[i] = inverseViewMatrix * cascadeFrustumCorners[i];
                boundingSphereCenter += Vector3f(cascadeFrustumCorners[i].x, cascadeFrustumCorners[i].y, cascadeFrustumCorners[i].z);
            }
            boundingSphereCenter /= 8.0f;

            float boundingSphereRadius = 0.0f;
            for (uint32 i = 0; i < 8; i++)
            {
                float distance = glm::length(Vector3f(cascadeFrustumCorners[i].x, cascadeFrustumCorners[i].y, cascadeFrustumCorners[i].z) - boundingSphereCenter);
                boundingSphereRadius = glm::max(boundingSphereRadius, distance);
            }
            boundingSphereRadius = std::ceil(boundingSphereRadius); // Use the ceilling function to increase stability.

            Vector4f boundingSphere = Vector4f(boundingSphereCenter.x, boundingSphereCenter.y, boundingSphereCenter.z, boundingSphereRadius);
#else
            Vector4f viewSpaceBoundingSphere = ComputeViewSpaceShadowCascadeMinimumBoundingSphere(cascadeStartDistance, cascadeEndDistance, tanHalfVerticalFOV, aspectRatio);
            float boundingSphereRadius = std::ceil(viewSpaceBoundingSphere.w); // Use the ceilling function to increase stability.

            Vector4f viewSpaceBoundingSphereCenter = Vector4f(viewSpaceBoundingSphere.x, viewSpaceBoundingSphere.y, viewSpaceBoundingSphere.z, 1.0f);

            Vector4f boundingSphere = inverseViewMatrix * viewSpaceBoundingSphereCenter;
            boundingSphere.w = boundingSphereRadius;

            Vector3f boundingSphereCenter = Vector3f(boundingSphere.x, boundingSphere.y, boundingSphere.z);

            // Scene Independent Projection
            // GPU Gems 3. Chapter 10. Parallel-Split Shadow Maps on Programmable GPUs
            // {
                //Vector4f viewSpaceBoundingSphereCenter = ;

                // To avoid shimmering caused by camera movements, create a "stable" projection using the method described in the article "Stable Cascaded Shadow Maps" from ShaderX6.
                // 1. Using a bounding sphere instead of a bounding box to guarantee the projection is rotation-invariant.
                // 2. Moving the shadow caster camera in texel-sized increments.

                //float snapX = std::fmodf(, 2.0f / shadowMapSize);
                //float snapY = std::fmodf(, 2.0f / shadowMapSize);
            // }

#endif
            float minZ = -100.0f;//-boundingSphereRadius;
            float maxZ = boundingSphereRadius;

            Matrix4x4f viewMatrix = glm::lookAt(boundingSphereCenter, boundingSphereCenter + lightDirection, Vector3f(0.0f, 1.0f, 0.0f));
            Matrix4x4f projectionMatrix = Math::OrthographicProjection_ReverseZ_ZO(-boundingSphereRadius, boundingSphereRadius, -boundingSphereRadius, boundingSphereRadius, minZ, maxZ);

            cascadeData.cascadeIndex = cascadeIndex;
            cascadeData.startDistance = cascadeStartDistance;
            cascadeData.endDistance = cascadeEndDistance;
            cascadeData.transitionRange = transitionRange;
            cascadeData.boundingSphere = boundingSphere;
            cascadeData.worldToViewMatrix = viewMatrix;
            cascadeData.viewToClipMatrix = projectionMatrix;
            cascadeData.worldToClipMatrix = projectionMatrix * viewMatrix;
        }
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
            shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(cascadeShadowMapDataBuffer));
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
}