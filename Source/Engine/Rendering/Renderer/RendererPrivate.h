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

namespace HE
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

    enum class ShaderPipelineID
    {
        PreIntegratedBRDF,
        EquirectangularToCubemap,
        DownsampleCubemap,
        DownsampleTexture2D,
        DownsampleTexture2D_PS,
        ComputeEnvironmentIrradiance,
        ComputeEnvironmentIrradianceSH,
        FilterEnvironmentMap,
        SharedMemoryComplexFFT,
        SharedMemoryComplexIFFT,
        SharedMemoryTwoForOneRealFFT,
        SharedMemoryTwoForOneRealIFFT,
        SharedMemoryComplexFFTConvolution,
        ImGui,
        Count,
    };

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

    struct LightInfo
    {
        const LightComponent* component;
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