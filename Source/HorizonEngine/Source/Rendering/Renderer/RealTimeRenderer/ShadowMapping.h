#pragma once

#include "RealTimeRenderer.h"

namespace Horizon
{
    enum ShadowMapType
    {
        CascadedShadowMap,
        VirtualShadowMap
    };

    struct ShadowMapCascadeData
    {
        uint32 cascadeIndex;
        float startDistance;
        float endDistance;
        float blendScale;
        float transitionRange;
        Vector4f boundingSphere;
        Matrix4x4f worldToViewMatrix;
        Matrix4x4f viewToClipMatrix;
        Matrix4x4f worldToClipMatrix;
    };

    struct CascadedShadowMapRenderData
    {
        uint32 resolution;
        uint32 cascadeCount;
        float shadowFadeOutRange;
        ShadowMapCascadeData cascadeData[RendererMaxShadowMapCascadeCount];
    };

    struct CascadedShadowMapShaderParameters
    {
        Matrix4x4f worldToClipMatrix[RendererMaxShadowMapCascadeCount];
        float cascadeEndDistance[RendererMaxShadowMapCascadeCount];
        float cascadeBlendScale[RendererMaxShadowMapCascadeCount];
        float transitionStartDistance[RendererMaxShadowMapCascadeCount];
        float inverseTransitionRange[RendererMaxShadowMapCascadeCount];
        Vector4f depthBiasParameters[RendererMaxShadowMapCascadeCount];
        Vector4f cascadeBoundingSphere[RendererMaxShadowMapCascadeCount];
        Vector2f shadowMapSize;
        Vector2f shadowFadeOutParameters;
        float maxShadowDistance;
        uint32 cascadeCount;
    };

    void SetupViewDependentCascadedShadowMapRenderDataForLight(CascadedShadowMapRenderData& outCascadedShadowMapRenderData, const SceneView& view, const LightRenderObject& light);

    void SetupCascadedShadowMapShaderParameters(CascadedShadowMapShaderParameters& outParameters, const SceneView& view, const LightRenderObject& light, const CascadedShadowMapRenderData& data);
}