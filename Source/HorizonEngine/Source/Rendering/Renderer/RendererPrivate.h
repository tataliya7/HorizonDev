#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    enum
    {
        RendererMaxLightCount = 128,
        RendererMaxShadowMapCascadeCount = 4,
        RendererMaxCascadedShadowMapCount = 2,
        RendererMaxCubeShadowMapCount = 8,
        RendererMaxMaterialTextureSlotCount = 16,
    };

    //static const Matrix4x4 CubeFaceMatrices[6] =
    //{
    //    /* +X */
    //    {
    //        {  0.0f,  0.0f, -1.0f,  0.0f },
    //        {  0.0f, -1.0f,  0.0f,  0.0f },
    //        { -1.0f,  0.0f,  0.0f,  0.0f },
    //        {  0.0f,  0.0f,  0.0f,  1.0f }
    //    },
    //    /* -X */
    //    {
    //        {  0.0f,  0.0f,  1.0f,  0.0f },
    //        {  0.0f, -1.0f,  0.0f,  0.0f },
    //        {  1.0f,  0.0f,  0.0f,  0.0f },
    //        {  0.0f,  0.0f,  0.0f,  1.0f }
    //    },
    //    /* +Y */
    //    {
    //        {  1.0f,  0.0f,  0.0f,  0.0f },
    //        {  0.0f,  0.0f, -1.0f,  0.0f },
    //        {  0.0f,  1.0f,  0.0f,  0.0f },
    //        {  0.0f,  0.0f,  0.0f,  1.0f }
    //    },
    //    /* -Y */
    //    {
    //        {  1.0f,  0.0f,  0.0f,  0.0f },
    //        {  0.0f,  0.0f,  1.0f,  0.0f },
    //        {  0.0f, -1.0f,  0.0f,  0.0f },
    //        {  0.0f,  0.0f,  0.0f,  1.0f }
    //    },
    //    /* +Z */
    //    {
    //        {  1.0f,  0.0f,  0.0f,  0.0f },
    //        {  0.0f, -1.0f,  0.0f,  0.0f },
    //        {  0.0f,  0.0f, -1.0f,  0.0f },
    //        {  0.0f,  0.0f,  0.0f,  1.0f }
    //    },
    //    /* -Z */
    //    {
    //        { -1.0f,  0.0f,  0.0f,  0.0f },
    //        {  0.0f, -1.0f,  0.0f,  0.0f },
    //        {  0.0f,  0.0f,  1.0f,  0.0f },
    //        {  0.0f,  0.0f,  0.0f,  1.0f }
    //    },
    //};

    struct CascadedShadowMapShaderParameters
    {
        Matrix4x4 viewProjectionMatrix[RendererMaxShadowMapCascadeCount];
        float cascadeEndDistance[RendererMaxShadowMapCascadeCount];
        float transitionStartDistance[RendererMaxShadowMapCascadeCount];
        float inverseTransitionRange[RendererMaxShadowMapCascadeCount];
        Vector2 shadowMapSize;
        Vector2 shadowFadeoutParameters;
        uint32 useTransition;
        uint32 cascadeCount;
    };

    struct CubeShadowMapShaderParameters
    {
        Matrix4x4 viewProjectionMatrix[6];
    };

    struct TextureShaderSlot
    {
        int bindlessTextureIndex = -1;

        inline bool IsValid()
        {
            return bindlessTextureIndex >= 0;
        }
    };

    enum
    {
        MATERIAL_FLAGS_BIT_USE_METALLIC_ROUGHNESS_WORKFLOW = (1 << 0),
    };

    struct MaterialShaderParameters
    {
        Vector4 baseColor;
        float metallic;
        float roughness;
        float specular;
        float specularTint;
        Vector4 emission;
        float emissionStrength;
        Vector4 sssSurfaceAlbedo;
        Vector4 sssMFP;
        float secondRoughness;
        float lobeMix;

        uint32 flags;
        TextureShaderSlot textures[RendererMaxMaterialTextureSlotCount];
    };

    // struct GeometryShaderParameters
    // {
    //     int vertexBuffer0;
    //     int vertexBuffer1;
    //     int vertexBuffer2;
    //     int vertexBuffer3;
    //     int prevVertexBuffer0;
    //     int indexBuffer;
    //     int transformBuffer;
    //     int previousTransformBuffer;
    //     int transformIndex = -1;
    //     int previousTransformIndex = -1;
    //     int materialIndexBuffer;
    //     int materialBufferOffset;
    //     uint32 baseVertex;
    //     uint32 vertexCount;
    //     uint32 baseIndex;
    //     uint32 indexCount;
    //     Vector3 boundsMin;
    //     Vector3 boundsMax;
    // };
}