#pragma once

namespace Horizon
{
    enum class SuperResolutionTechnique
    {
        None,
        FSR2,
        DLSSSuperResolution,
    };

    enum class FrameRateUpConversionTechnique
    {
        None,
        DLSSFrameGeneration,
    };

    enum class AntialiasingTechnique
    {
        None,
        TemporalAA,
        DLAA,
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
        Low = 0,
        Medium = 1,
        High = 2,
        Epic = 3,
    };

    struct ScreenSpaceReflectionsSettings
    {
        bool denosingEnabled = true;
        ScreenSpaceReflectionsQuality qualiy = ScreenSpaceReflectionsQuality::Epic;
    };

    enum class PostProcessingLensFlaresQuality
    {
        Disabled,
        Low,
        High,
        VeryHigh,
    };

    enum class DLSSQualityMode
    {
        Off = 0,
        Auto = 1,
        Quality = 2,
        Balanced = 3,
        Performance = 4,
        UltraPerformance = 5,
    };

    struct DLSSSettings
    {
        DLSSQualityMode qualityMode = DLSSQualityMode::Auto;
    };

    enum class NVIDIAReflexMode
    {
        Off = 0,
        LowLatency = 1,
        LowLatencyWithBoost = 2,
    };

    struct RenderSettings
    {
        RendererType rendererType;
        bool fixedPreExposureEnabled = false;
        float fixedPreExposure = 1.0f;
        Vector3 indirectLightingColor = Vector3(1.0f, 1.0f, 1.0f);
        float indirectLightingIntensity = 1.0f;
        float upscaleRatio = 1.0f;
        bool forceEnableSubpixelJittering = false;
        bool enableShadowsDenoiser = false;
        ShadowsTechnique shadowsTechnique = ShadowsTechnique::None;
        ReflectionsTechnique reflectionsTechnique = ReflectionsTechnique::None;
        AmbientOcclusionTechnique ambientOcclusionTechnique = AmbientOcclusionTechnique::None;
        AntialiasingTechnique antialiasingTechnique = AntialiasingTechnique::None;
        SuperResolutionTechnique superResolutionTechnique = SuperResolutionTechnique::None;
        ScreenSpaceReflectionsSettings ssrSettings;
        GroundTruthAmbientOcclusionSettings gtaoSettings;
        FSR2Settings fsr2Settings;
        DLSSSettings dlssSettings;
        NVIDIAReflexMode reflexMode = NVIDIAReflexMode::LowLatency;
        PostProcessingSettings postProcessingSettings;
    };
}