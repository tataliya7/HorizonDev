#pragma once

#include "Core/CoreModule.h"

enum
{
    RendererMaxLightCount = 128,
    RendererMaxShadowMapCascadeCount = 4,
    RendererMaxCascadedShadowMapCount = 2,
    RendererMaxCubeShadowMapCount = 8,
    RendererMaxMaterialTextureSlotCount = 16,
};

namespace Horizon
{
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

    void JitterProjectionMatrix(Matrix4x4& outProjectionMatrix, Vector2& outJitterOffset, const Extent2D& renderResolution, float upscaleRatio)
    {
        static const auto HaltonSequence = [](uint32 index, uint32 base)
            {
                float f = 1.0f, result = 0.0f;
                for (uint32 i = index; i > 0;)
                {
                    f /= static_cast<float>(base);
                    result = result + f * static_cast<float>(i % base);
                    i = static_cast<uint32>(floorf(static_cast<float>(i) / static_cast<float>(base)));
                }
                return result;
            };

        uint32 temporalSampleCount = 32;
        temporalSampleCount = uint32(float(temporalSampleCount) * std::max(1.0f, upscaleRatio * upscaleRatio));
        temporalSampleCount = Math::Clamp(temporalSampleCount, 1, 255);

        static uint32 temporalSampleIndex = 0;
        temporalSampleIndex = (temporalSampleIndex + 1) % temporalSampleCount;

        // Unit pixel space offset
        float offsetX = HaltonSequence(temporalSampleIndex + 1, 2) - 0.5f;
        float offsetY = HaltonSequence(temporalSampleIndex + 1, 3) - 0.5f;

        // Clip space offset [-1, 1]
        // -y for clip space to uv space
        Vector2 jitterOffset = { offsetX * 2.0f / (float)renderResolution.width, -offsetY * 2.0f / (float)renderResolution.height };

        /*
         * Horizon Engine uses righted-handed coordinate system,
         * the w component of clip space position is -Zc instead of Zc,
         * so it should be multiplied by -1.
         */
        outProjectionMatrix[2][0] += -jitterOffset.x;
        outProjectionMatrix[2][1] += -jitterOffset.y;
        outJitterOffset = { offsetX, offsetY };
    }

    struct CascadedShadowMapShaderParameters
    {
        Matrix4x4 viewProjectionMatrix[RendererMaxShadowMapCascadeCount];
        float splitDepth[RendererMaxShadowMapCascadeCount];
        uint32 numCascades;
    };

    struct CubeShadowMapShaderParameters
    {
        Matrix4x4 viewProjectionMatrix[6];
    };

    struct LightShaderParameters
    {
        Vector3 color;
        Vector3 position;
        Vector3 forwardVec;
        Vector3 rightVec;
        Vector3 upVec;
        uint32 type;
        float radius;
        float sizeX;
        float sizeY;
        int32 shadowMapIndex;
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

    struct GeometryShaderParameters
    {
        int vertexBuffer0;
        int vertexBuffer1;
        int vertexBuffer2;
        int vertexBuffer3;
        int prevVertexBuffer0;
        int indexBuffer;
        int transformBuffer;
        int previousTransformBuffer;
        int transformIndex = -1;
        int previousTransformIndex = -1;
        int materialIndexBuffer;
        int materialBufferOffset;
        uint32 baseVertex;
        uint32 vertexCount;
        uint32 baseIndex;
        uint32 indexCount;
        Vector3 boundsMin;
        Vector3 boundsMax;
    };
}