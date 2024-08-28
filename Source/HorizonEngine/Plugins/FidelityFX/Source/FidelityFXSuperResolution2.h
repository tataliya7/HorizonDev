#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

namespace Horizon
{
    struct FidelityFxSuperResolution2State;

    enum class FidelityFXSuperResolution2API
    {
        Unknown,
        D3D12,
        Vulkan,
    };

    enum class FidelityFXSuperResolution2QualityMode
    {
        Quality,
        Balanced,
        Performance,
        UltraPerformance,
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

    class FidelityFXSuperResolution2 : public TemporalSuperSamplingInterface
    {
    public:
        FidelityFXSuperResolution2(RenderBackend* renderBackend);
        ~FidelityFXSuperResolution2();
        TemporalSuperSamplingConstants GetConstants() const
        {
            return constants;
        }
        void SetOptions(const TemporalSuperSamplingOptions& options) override;
        void SetConstants(const TemporalSuperSamplingConstants& constants) override;
        TemporalSuperSamplingOptimalSettings GetOptimalSettings() const override;
        uint32 GetJitterPhaseCount(uint32 renderWidth, uint32 targetWidth) const override;
        Vector2 GetJitterOffset(uint32 index, uint32 phaseCount) const override;
        RenderGraphTextureHandle Dispatch(RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchDescription& dispatchDescription) override;

    private:

        friend bool FidelityFXSuperResolution2DispatchD3D12(
            void* commandList,
            void* context,
            const RenderBackendTextureResource& output,
            const RenderBackendTextureResource& color,
            const RenderBackendTextureResource& depth,
            const RenderBackendTextureResource& motionVectors,
            const RenderBackendTextureResource& exposure);

        friend bool FidelityFXSuperResolution2DispatchVulkan(
            void* commandList,
            void* context,
            const RenderBackendTextureResource& output,
            const RenderBackendTextureResource& color,
            const RenderBackendTextureResource& depth,
            const RenderBackendTextureResource& motionVectors,
            const RenderBackendTextureResource& exposure);

        RenderBackend* renderBackend;
        FidelityFXSuperResolution2API api;
        FidelityFxSuperResolution2State* state;
        TemporalSuperSamplingOptions options;
        TemporalSuperSamplingConstants constants;
    };
}