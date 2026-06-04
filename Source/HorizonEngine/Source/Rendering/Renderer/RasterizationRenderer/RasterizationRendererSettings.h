#pragma once

#include "Rendering/Renderer/PostProcessing/PostProcessingSettings.h"

namespace Horizon
{
    enum class RasterizationRendererDebugVisualizationMode
    {
        Lighting,
        Wireframe,
        Illuminance,
        Depth,
        WorldSpaceNormal,
        MeshletID,
        PrimitiveID,
        MaterialID,
        MotionVectors,
        AmbientOcclusion,
        ShadowMask,
        CascadedShadowMapCascadeIndex,
        VirtualShadowMapMipmap,
        VirtualShadowMapVirtualPage,
    };

    static const char* ViewModeName[] =
    {
        "Lighting",
        "Wireframe",
        "Illuminance",
        "Linear Depth",
        "World Space Normal",
        "Meshlet ID",
        "Primitive ID",
        "Material ID",
        "Motion Vectors",
        "Ambient Occlusion",
        "Screen Space Shadow Mask",
        "Cascaded Shadow Map Cascade Index",
        "Virtual Shadow Map Mipmap",
        "Virtual Shadow Map Page",
    };
    static_assert(ArraySize(ViewModeName) == 14);

    enum class SuperSamplingTechnique
    {
        None,
        TSS,
        FSR2,
        FSR3,
        DLSS,
    };

    struct RasterizationRendererSuperSamplingSettings
    {
        SuperSamplingTechnique superSamplingTechnique = SuperSamplingTechnique::None;
        uint32 qualityMode = 0;
        float desiredRenderResolutionPercentage = 1.0f;
        bool enableFrameInterpolation = false;
    };

    enum class RasterizationRendererShadowsTechnique
    {
        None,
        ShadowMaps,
        VirtualShadowMaps,
        RayTracingShadows,
    };

    enum class RasterizationRendererReflectionsTechnique
    {
        None,
        ScreenSpaceReflections,
        RayTracingReflections,
    };

    enum class RasterizationRendererAmbientOcclusionTechnique
    {
        None,
        GroundTruthAmbientOcclusion,
        RayTracingAmbientOcclusion,
    };

    struct RasterizationRendererGroundTruthAmbientOcclusionSettings
    {
        float strength = 1.0f;
        float radius = 2.0f;
        float thickness = 0.75f;
    };

    enum class ScreenSpaceReflectionsQuality
    {
        Off,
        Low,
        Medium,
        High,
        Epic,
    };

    struct RasterizationRendererScreenSpaceReflectionsSettings
    {
        bool enableDenoising = true;
        ScreenSpaceReflectionsQuality quality = ScreenSpaceReflectionsQuality::High;
    };

    enum class RasterizationRendererGlobalIlluminationTechnique
    {
        None,
        Surfel,
    };

    struct RasterizationRendererGlobalIlluminationSettings
    {
        Vector3f indirectLightingColor = Vector3f(1.0f, 1.0f, 1.0f);
        float indirectLightingIntensity = 1.0f;
    };

    struct RasterizationRendererDeveloperSettings
    {

    };

    struct RasterizationRendererSettings
    {
        RasterizationRendererDebugVisualizationMode debugVisualizationMode;
        bool enableFixedPreExposure = false;
        float fixedPreExposure = 1.0f;
        RasterizationRendererGlobalIlluminationSettings globalIlluminationSettings;
        RasterizationRendererShadowsTechnique shadowsTechnique;
        RasterizationRendererReflectionsTechnique reflectionsTechnique;
        RasterizationRendererScreenSpaceReflectionsSettings screenSpaceReflectionsSettings;
        RasterizationRendererAmbientOcclusionTechnique ambientOcclusionTechnique;
        RasterizationRendererGroundTruthAmbientOcclusionSettings groundTruthAmbientOcclusionSettings;
        RasterizationRendererSuperSamplingSettings superSamplingSettings;
        PostProcessingSettings postProcessingSettings;
        RasterizationRendererDeveloperSettings developerSettings;
        bool enableAsyncCompute = false;
    };
}