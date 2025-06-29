module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

export module FidelityFX.FSR2;

export namespace Horizon
{
    enum class FidelityFXSuperResolution2API
    {
        Unknown,
        D3D12,
        Vulkan,
    };

    enum class FidelityFXSuperResolution2QualityMode
    {
        Off,
        Quality,
        Balanced,
        Performance,
        UltraPerformance,
        Custom
    };

    struct FidelityFXSuperResolution2Settings
    {
        bool enabled = false;
        bool enableSharpening = false;
        bool overrideRenderResolutionPercentage = false;
        float sharpness = 1.0f;
        float renderResolutionPercentage = 1.0f;
        FidelityFXSuperResolution2QualityMode qualityMode = FidelityFXSuperResolution2QualityMode::Quality;
    };

    TemporalSuperSamplingInterface* FidelityFXSuperResolution2Create(RenderBackend* renderBackend);

    void FidelityFXSuperResolution2Destroy(TemporalSuperSamplingInterface* temporalSuperSamplingInterface);
}