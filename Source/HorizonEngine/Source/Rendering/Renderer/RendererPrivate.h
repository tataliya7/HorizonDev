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
        RendererMaxAtmosphericLightCount = 2,
        TriangleWindingOrder = 0,
    };

    //static const Matrix4x4f CubeFaceMatrices[6] =
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

    struct CubeShadowMapShaderParameters
    {
        Matrix4x4f viewProjectionMatrix[6];
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
        Vector4f baseColor;
        float metallic;
        float roughness;
        float specular;
        float specularTint;
        Vector4f emission;
        float emissionStrength;
        Vector4f sssSurfaceAlbedo;
        Vector4f sssMFP;
        float secondRoughness;
        float lobeMix;

        uint32 flags;
        TextureShaderSlot textures[RendererMaxMaterialTextureSlotCount];
    };
}