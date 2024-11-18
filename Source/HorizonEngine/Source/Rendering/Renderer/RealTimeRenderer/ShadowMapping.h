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
        float transitionRange;
        Vector4f boundingSphere;
        Matrix4x4f worldToViewMatrix;
        Matrix4x4f viewToClipMatrix;
        Matrix4x4f worldToClipMatrix;
    };

    struct CascadedShadowMapShaderParameters
    {
        Matrix4x4 worldToClipMatrix[RendererMaxShadowMapCascadeCount];
        float cascadeEndDistance[RendererMaxShadowMapCascadeCount];
        float transitionStartDistance[RendererMaxShadowMapCascadeCount];
        float inverseTransitionRange[RendererMaxShadowMapCascadeCount];
        Vector2 shadowMapSize;
        Vector2 shadowFadeOutParameters;
        float maxShadowDistance;
        uint32 cascadeCount;
    };

    struct CascadedShadowMapRenderData
    {
        uint32 resolution;
        uint32 cascadeCount;
        float shadowFadeOutRange;
        ShadowMapCascadeData cascadeData[RendererMaxShadowMapCascadeCount];
    };

    void SetupViewDependentCascadedShadowMapRenderDataForLight(CascadedShadowMapRenderData& outCascadedShadowMapRenderData, const SceneView& view, const LightRenderObject& light);

    void SetupCascadedShadowMapShaderParameters(CascadedShadowMapShaderParameters& outParameters, const SceneView& view, const LightRenderObject& light, const CascadedShadowMapRenderData& data);
}