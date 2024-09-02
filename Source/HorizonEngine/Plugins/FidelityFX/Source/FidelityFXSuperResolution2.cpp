module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

#include <ffx_fsr2.h>

module FidelityFX.SuperResolution2;

import :D3D12;
import :Vulkan;

namespace Horizon
{
    TemporalSuperSamplingInterface* FidelityFXSuperResolution2Create(RenderBackend* renderBackend)
    {
        return new FidelityFXSuperResolution2(renderBackend);
    }

    void FidelityFXSuperResolution2Destroy(TemporalSuperSamplingInterface* interface)
    {
        delete reinterpret_cast<FidelityFXSuperResolution2*>(interface);
    }

    static FfxFsr2QualityMode GetFfxFsr2QualityMode(FidelityFXSuperResolution2QualityMode qualityMode)
    {
        switch (qualityMode)
        {
        case FidelityFXSuperResolution2QualityMode::Quality:
            return FFX_FSR2_QUALITY_MODE_QUALITY;
        case FidelityFXSuperResolution2QualityMode::Balanced:
            return FFX_FSR2_QUALITY_MODE_BALANCED;
        case FidelityFXSuperResolution2QualityMode::Performance:
            return FFX_FSR2_QUALITY_MODE_PERFORMANCE;
        case FidelityFXSuperResolution2QualityMode::UltraPerformance:
            return FFX_FSR2_QUALITY_MODE_ULTRA_PERFORMANCE;
        }
        std::unreachable();
    }

    static FidelityFXSuperResolution2API GetFidelityFXSuperResolution2API(RenderBackendType renderBackendType)
    {
        switch (renderBackendType)
        {
        case RenderBackendType::D3D12:
            return FidelityFXSuperResolution2API::D3D12;
        case RenderBackendType::Vulkan:
            return FidelityFXSuperResolution2API::Vulkan;
        default:
            return FidelityFXSuperResolution2API::Unknown;
        }
    }

    FidelityFXSuperResolution2::FidelityFXSuperResolution2(RenderBackend* renderBackend)
        : renderBackend(renderBackend)
        , api(GetFidelityFXSuperResolution2API(renderBackend->GetType()))
        , state(nullptr)
    {
        state = new FidelityFxSuperResolution2State();
        state->initialized = false;
    }

    FidelityFXSuperResolution2::~FidelityFXSuperResolution2()
    {
        if (state != nullptr)
        {
            if (state->fsr2ContextDescription.callbacks.scratchBuffer != nullptr)
            {
                ffxFsr2ContextDestroy(&state->fsr2Context);
                free(state->fsr2ContextDescription.callbacks.scratchBuffer);
                state->fsr2ContextDescription.callbacks.scratchBuffer = nullptr;
            }

            delete state;
            state = nullptr;
        }
    }

    void FidelityFXSuperResolution2::SetOptions(const TemporalSuperSamplingOptions& options)
    {
        this->options = options;
    }

    void FidelityFXSuperResolution2::SetConstants(const TemporalSuperSamplingConstants& constants)
    {
        this->constants = constants;
    }

    TemporalSuperSamplingOptimalSettings FidelityFXSuperResolution2::GetOptimalSettings() const
    {
        uint32_t targetWidth = options.targetWidth;
        uint32_t targetHeight = options.targetHeight;
        uint32_t renderWidth = 0;
        uint32_t renderHeight = 0;
        float renderResolutionPercentage = 1.0f;

#if 0
        if (false)
        {
            renderResolutionPercentage = ;

            renderWidth = uint32_t(renderResolutionPercentage * float(targetWidth));
            renderHeight = uint32_t(renderResolutionPercentage * float(targetHeight));
        }
        else
#endif
        {
            // TODO: Implement FSR2 quality mode selection.
            //FfxFsr2QualityMode fsr2QualityMode = GetFfxFsr2QualityMode();
            FfxFsr2QualityMode fsr2QualityMode = FFX_FSR2_QUALITY_MODE_QUALITY;

            FfxErrorCode errorCode = ffxFsr2GetRenderResolutionFromQualityMode(
                &renderWidth,
                &renderHeight,
                targetWidth,
                targetHeight,
                fsr2QualityMode);
            FFX_ASSERT(errorCode == FFX_OK);

            float upscaleRatio = ffxFsr2GetUpscaleRatioFromQualityMode(fsr2QualityMode);
            renderResolutionPercentage = 1.0f / upscaleRatio;
        }

        TemporalSuperSamplingOptimalSettings optimalSettings = {};
        optimalSettings.optimalRenderWidth = renderWidth;
        optimalSettings.optimalRenderHeight = renderHeight;
        optimalSettings.optimalRenderResolutionPercentage = renderResolutionPercentage;

        return optimalSettings;
    }

    uint32 FidelityFXSuperResolution2::GetJitterPhaseCount(uint32 renderWidth, uint32 targetWidth) const
    {
        const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount(int32_t(renderWidth), int32_t(targetWidth));
        return uint32(jitterPhaseCount);
    }

    Vector2 FidelityFXSuperResolution2::GetJitterOffset(uint32 index, uint32 phaseCount) const
    {
        // TODO: Is it safe to cast index and phaseCount to int32_t?
        Vector2 jitterOffset(0.0f, 0.0f);
        FfxErrorCode errorCode = ffxFsr2GetJitterOffset(&jitterOffset.x, &jitterOffset.y, int32_t(index), int32_t(phaseCount));
        FFX_ASSERT(errorCode == FFX_OK);
        return jitterOffset;
    }

    RenderGraphTextureHandle FidelityFXSuperResolution2::Dispatch(
        RenderGraph& renderGraph,
        const SceneView& view,
        const TemporalSuperSamplingDispatchDescription& dispatchDescription)
    {
        RenderBackendDispatchSuperSamplingCallback dispatchCallback = nullptr;
        switch (api)
        {
        case FidelityFXSuperResolution2API::D3D12: dispatchCallback = FidelityFXSuperResolution2DispatchD3D12; break;
        case FidelityFXSuperResolution2API::Vulkan: dispatchCallback = FidelityFXSuperResolution2DispatchVulkan; break;
        default: std::unreachable(); break;
        }

        RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
                view.targetWidth,
                view.targetHeight,
                RenderBackendTextureFormat::R16G16B16A16Float,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "FSR2OutputTexture");

        renderGraph.AddPass(
            std::format("FidelityFXSuperResolution2Dispatch (Compute, {}x{} -> {}x{})", renderGraph.GetTextureDesc(dispatchDescription.colorTexture).width, renderGraph.GetTextureDesc(dispatchDescription.colorTexture).height, outputTextureDesc.width, outputTextureDesc.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle colorTexture = builder.ReadTexture(dispatchDescription.colorTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle depthTexture = builder.ReadTexture(dispatchDescription.depthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle motionVectorTexture = builder.ReadTexture(dispatchDescription.motionVectorTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle exposureTexture = builder.ReadTexture(dispatchDescription.exposureTexture, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.DispatchSuperSampling(
                        this,
                        dispatchCallback,
                        registry.GetRenderBackendTextureHandle(outputTexture),
                        registry.GetRenderBackendTextureHandle(colorTexture),
                        registry.GetRenderBackendTextureHandle(depthTexture),
                        registry.GetRenderBackendTextureHandle(motionVectorTexture),
                        registry.GetRenderBackendTextureHandle(exposureTexture));
                };
            });

        return outputTexture;
    }
}