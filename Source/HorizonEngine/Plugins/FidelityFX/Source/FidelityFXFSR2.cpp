module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

#include <FidelityFX/host/ffx_fsr2.h>

module FidelityFX.FSR2;

import :D3D12;
import :Vulkan;

namespace Horizon
{
    TemporalSuperSamplingInterface* FidelityFXSuperResolution2Create(RenderBackend* renderBackend)
    {
        return new FidelityFXFSR2(renderBackend);
    }

    void FidelityFXSuperResolution2Destroy(TemporalSuperSamplingInterface* temporalSuperSamplingInterface)
    {
        delete reinterpret_cast<FidelityFXFSR2*>(temporalSuperSamplingInterface);
    }

    static FfxFsr2QualityMode GetFfxFsr2QualityMode(uint32 qualityMode)
    {
        switch (qualityMode)
        {
        case 1:
            return FFX_FSR2_QUALITY_MODE_QUALITY;
        case 2:
            return FFX_FSR2_QUALITY_MODE_BALANCED;
        case 3:
            return FFX_FSR2_QUALITY_MODE_PERFORMANCE;
        case 4:
            return FFX_FSR2_QUALITY_MODE_ULTRA_PERFORMANCE;
        default:
            std::unreachable();
            return FFX_FSR2_QUALITY_MODE_QUALITY;
        }
    }

    static FidelityFXSuperResolution2API GetFidelityFXSuperResolution2API(RenderBackendType renderBackendType)
    {
        switch (renderBackendType)
        {
        case RenderBackendType::Direct3D12:
            return FidelityFXSuperResolution2API::D3D12;
        case RenderBackendType::Vulkan:
            return FidelityFXSuperResolution2API::Vulkan;
        default:
            return FidelityFXSuperResolution2API::Unknown;
        }
    }

    FidelityFXFSR2::FidelityFXFSR2(RenderBackend* renderBackend)
        : renderBackend(renderBackend)
        , api(GetFidelityFXSuperResolution2API(renderBackend->GetType()))
        , state(nullptr)
        , options()
        , constants()
    {
        state = new FidelityFXSuperResolution2State();
        state->fsr2ContextDescription = {};
        state->fsr2Context = {};
        state->initialized = false;
    }

    FidelityFXFSR2::~FidelityFXFSR2()
    {
        if (state != nullptr)
        {
            if (state->fsr2ContextDescription.backendInterface.scratchBuffer != nullptr)
            {
                ffxFsr2ContextDestroy(&state->fsr2Context);
                free(state->fsr2ContextDescription.backendInterface.scratchBuffer);
                state->fsr2ContextDescription.backendInterface.scratchBuffer = nullptr;
            }

            delete state;
            state = nullptr;
        }
    }

    void FidelityFXFSR2::SetOptions(const TemporalSuperSamplingOptions& options)
    {
        this->options = options;
    }

    void FidelityFXFSR2::SetConstants(const TemporalSuperSamplingConstants& constants)
    {
        this->constants = constants;
    }

    TemporalSuperSamplingOptimalSettings FidelityFXFSR2::GetOptimalSettings() const
    {
        uint32_t targetWidth = options.outputWidth;
        uint32_t targetHeight = options.outputHeight;
        uint32_t renderWidth = 0;
        uint32_t renderHeight = 0;
        float renderResolutionPercentage = 1.0f;

        if (options.qualityMode < FFX_FSR2_QUALITY_MODE_QUALITY || options.qualityMode > FFX_FSR2_QUALITY_MODE_ULTRA_PERFORMANCE)
        {
            renderResolutionPercentage = std::clamp(options.desiredRenderResolutionPercentage, SuperResolutionMinPercentage, SuperResolutionMaxPercentage);

            renderWidth = uint32_t(renderResolutionPercentage * float(targetWidth));
            renderHeight = uint32_t(renderResolutionPercentage * float(targetHeight));
        }
        else
        {
            FfxFsr2QualityMode fsr2QualityMode = GetFfxFsr2QualityMode(options.qualityMode);

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

        TemporalSuperSamplingOptimalSettings optimalSettings =
        {
            .optimalRenderWidth = renderWidth,
            .optimalRenderHeight = renderHeight,
            .optimalRenderResolutionPercentage = renderResolutionPercentage
        };

        return optimalSettings;
    }

    uint32 FidelityFXFSR2::GetJitterPhaseCount(uint32 renderWidth, uint32 targetWidth) const
    {
        const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount(int32_t(renderWidth), int32_t(targetWidth));
        return uint32(jitterPhaseCount);
    }

    Vector2f FidelityFXFSR2::GetJitterOffset(uint32 index, uint32 phaseCount) const
    {
        // TODO: Is it safe to cast index and phaseCount to int32_t?
        Vector2f jitterOffset(0.0f, 0.0f);
        FfxErrorCode errorCode = ffxFsr2GetJitterOffset(&jitterOffset.x, &jitterOffset.y, int32_t(index), int32_t(phaseCount));
        FFX_ASSERT(errorCode == FFX_OK);
        return jitterOffset;
    }

    RenderGraphTextureHandle FidelityFXFSR2::Dispatch(
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

        void* fsrContext = this;

        const RenderGraphTextureDescription& colorTextureDescription = renderGraph.GetTextureDescription(dispatchDescription.colorTexture);

        RenderGraphTextureDescription outputTextureDescription = RenderGraphTextureDescription::Create2D(
                view.targetWidth,
                view.targetHeight,
                RenderBackendTextureFormat::R16G16B16A16Float,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDescription, "FSR2OutputTexture");

        renderGraph.AddPass(
            std::format("FidelityFXSuperResolution2Dispatch (Compute, {}x{} -> {}x{})", colorTextureDescription.width, colorTextureDescription.height, outputTextureDescription.width, outputTextureDescription.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle colorTexture = builder.ReadTexture(dispatchDescription.colorTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle depthTexture = builder.ReadTexture(dispatchDescription.depthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle motionVectorTexture = builder.ReadTexture(dispatchDescription.motionVectorTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle exposureTexture = builder.ReadTexture(dispatchDescription.exposureTexture, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.DispatchSuperSampling(
                        fsrContext,
                        dispatchCallback,
                        resourceRegistry.GetRenderBackendTextureHandle(outputTexture),
                        resourceRegistry.GetRenderBackendTextureHandle(colorTexture),
                        resourceRegistry.GetRenderBackendTextureHandle(depthTexture),
                        resourceRegistry.GetRenderBackendTextureHandle(motionVectorTexture),
                        resourceRegistry.GetRenderBackendTextureHandle(exposureTexture));
                };
            });

        return outputTexture;
    }
}