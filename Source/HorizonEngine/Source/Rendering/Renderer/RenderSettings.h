#pragma once

#include "RendererCommon.h"
#include "PostProcessingSettings.h"
#include "SceneRenderer.h"

namespace Horizon
{
    enum class SuperSamplingTechnique
    {
        None,
        //TSS,
        FSR,
        DLSS,
    };

    struct SuperSamplingSettings
    {
        SuperSamplingTechnique superSamplingTechnique = SuperSamplingTechnique::None;
        uint32 qualityMode = 0; // 0 means off.
        float desiredRenderResolutionPercentage = 1.0f;
        bool enableFrameInterpolation = false;
    };

    enum class ShadowsTechnique
    {
        None,
        ShadowMap,
        VirtualShadowMap,
        RayTracingShadows,
    };

    enum class ReflectionsTechnique
    {
        None,
        ScreenSpaceReflections,
        RayTracingReflections,
    };

    enum class AmbientOcclusionTechnique
    {
        None,
        GroundTruthAmbientOcclusion,
        RayTracingAmbientOcclusion,
    };

    struct GroundTruthAmbientOcclusionSettings
    {
        float radius = 0.2f;
        float factor = 1.0f;
        float thickness = 0.75f;
        bool multiBounce = true;
    };

    enum class ScreenSpaceReflectionsQuality
    {
        Off,
        Low,
        Medium,
        High,
        Epic,
    };

    struct ScreenSpaceReflectionsSettings
    {
        bool enableDenoising = true;
        ScreenSpaceReflectionsQuality quality = ScreenSpaceReflectionsQuality::High;
    };

    // enum class PostProcessingLensFlareQuality
    // {
    //     Off,
    //     Low,
    //     Medium,
    //     High,
    // };

    enum class GlobalIlluminationTechnique
    {
        None,
        Surfel,
    };

    struct GlobalIlluminationSettings
    {
        Vector3f indirectLightingColor = Vector3f(1.0f, 1.0f, 1.0f);
        float indirectLightingIntensity = 1.0f;
    };

    /**
     * TBD.
     */
    struct RenderSettings
    {
        RenderMode renderMode = RenderMode::Rasterization;
        bool enableFixedPreExposure = false;
        float fixedPreExposure = 1.0f;
        GlobalIlluminationSettings globalIlluminationSettings;
        SuperSamplingSettings superSamplingSettings;
        ShadowsTechnique shadowsTechnique;
        ReflectionsTechnique reflectionsTechnique;
        AmbientOcclusionTechnique ambientOcclusionTechnique;
        ScreenSpaceReflectionsSettings ssrSettings;
        GroundTruthAmbientOcclusionSettings gtaoSettings;
        PostProcessingSettings postProcessingSettings;
        bool enableAsyncCompute = false;
    };
}