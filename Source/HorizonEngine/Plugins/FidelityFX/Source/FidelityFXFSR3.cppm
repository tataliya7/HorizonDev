module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

export module FidelityFX.FSR3;

export namespace Horizon
{
    enum class FidelityFXFSR3API
    {
        Unknown,
        D3D12,
        Vulkan,
    };

    enum class FidelityFXFSR3QualityMode
    {
        Off,
        Quality,
        Balanced,
        Performance,
        UltraPerformance,
        Custom
    };

    struct FSR3Settings
    {
        bool enabled = false;
        bool enableSharpening = false;
        bool overrideRenderResolutionPercentage = false;
        float sharpness = 1.0f;
        float renderResolutionPercentage = 1.0f;
        FidelityFXFSR3QualityMode qualityMode = FidelityFXFSR3QualityMode::Quality;
    };

    TemporalSuperSamplingInterface* FidelityFXFSR3Create(RenderBackend* renderBackend);

    void FidelityFXFSR3Destroy(TemporalSuperSamplingInterface* temporalSuperSamplingInterface);
}