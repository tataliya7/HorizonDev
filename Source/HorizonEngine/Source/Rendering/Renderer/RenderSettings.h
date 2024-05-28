#pragma once

#include "RendererCommon.h"
#include "PostProcessingSettings.h"
#include "SceneRenderer.h"

namespace Horizon
{
    enum class FrameRateUpConversionTechnique
    {
        None,
        ThirdParty,
    };

    enum class SuperSamplingTechnique
    {
        None,
        SuperSamplingAntiAliasing,
        TemporalSuperSampling,
        ThirdParty,
    };

    enum class ShadowsTechnique
    {
        None,
        ScreenSpaceShadows,
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
        bool denoisingEnabled = true;
        ScreenSpaceReflectionsQuality quality = ScreenSpaceReflectionsQuality::High;
    };

    enum class PostProcessingLensFlaresQuality
    {
        Off,
        Low,
        Medium,
        High,
    };

    struct GlobalIlluminationSettings
    {
        Vector3 indirectLightingColor = Vector3(1.0f, 1.0f, 1.0f);
        float indirectLightingIntensity = 1.0f;
    };

    /**
     * TBD.
     */
    struct RenderSettings
    {
        RendererType rendererType;
        float upscaleRatio;
        GlobalIlluminationSettings globalIlluminationSettings;
        ShadowsTechnique shadowsTechnique;
        ReflectionsTechnique reflectionsTechnique;
        AmbientOcclusionTechnique ambientOcclusionTechnique;
        SuperSamplingTechnique superSamplingTechnique;
        ScreenSpaceReflectionsSettings ssrSettings;
        GroundTruthAmbientOcclusionSettings gtaoSettings;
        PostProcessingSettings postProcessingSettings;
    };
}