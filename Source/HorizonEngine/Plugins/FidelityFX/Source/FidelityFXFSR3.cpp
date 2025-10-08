module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

#include <FidelityFX/host/ffx_fsr3.h>

module FidelityFX.FSR3;

import :D3D12;
import :Vulkan;

namespace Horizon
{
    TemporalSuperSamplingInterface* FidelityFXFSR3Create(RenderBackend* renderBackend)
    {
        return new FidelityFXFSR3(renderBackend);
    }

    void FidelityFXFSR3Destroy(TemporalSuperSamplingInterface* temporalSuperSamplingInterface)
    {
        delete reinterpret_cast<FidelityFXFSR3*>(temporalSuperSamplingInterface);
    }

    static FfxFsr3QualityMode GetFfxFsr3QualityMode(uint32 qualityMode)
    {
        switch (qualityMode)
        {
        case 1:
            return FFX_FSR3_QUALITY_MODE_QUALITY;
        case 2:
            return FFX_FSR3_QUALITY_MODE_BALANCED;
        case 3:
            return FFX_FSR3_QUALITY_MODE_PERFORMANCE;
        case 4:
            return FFX_FSR3_QUALITY_MODE_ULTRA_PERFORMANCE;
        default:
            std::unreachable();
            return FFX_FSR3_QUALITY_MODE_QUALITY;
        }
    }

    static FidelityFXFSR3API GetFidelityFXFSR3API(RenderBackendType renderBackendType)
    {
        switch (renderBackendType)
        {
        case RenderBackendType::Direct3D12:
            return FidelityFXFSR3API::D3D12;
        case RenderBackendType::Vulkan:
            return FidelityFXFSR3API::Vulkan;
        default:
            return FidelityFXFSR3API::Unknown;
        }
    }

    FidelityFXFSR3::FidelityFXFSR3(RenderBackend* renderBackend)
        : renderBackend(renderBackend)
        , api(GetFidelityFXFSR3API(renderBackend->GetType()))
        , state()
        , options()
        , constants()
    {
        state.fsr3ContextDescription = {};
        state.fsr3Context = new FfxFsr3Context();
        state.initialized = false;

        //memset(state.fsr3Context, 0, sizeof(state.fsr3Context));
    }

    FidelityFXFSR3::~FidelityFXFSR3()
    {
        if (state.initialized)
        {
            if (state.fsr3ContextDescription.backendInterfaceUpscaling.scratchBuffer != nullptr)
            {
                ffxFsr3ContextDestroy(state.fsr3Context);
                free(state.fsr3ContextDescription.backendInterfaceUpscaling.scratchBuffer);
                state.fsr3ContextDescription.backendInterfaceUpscaling.scratchBuffer = nullptr;
            }

            state.initialized = false;
        }
    }

    void FidelityFXFSR3::SetOptions(const TemporalSuperSamplingOptions& options)
    {
        this->options = options;
    }

    void FidelityFXFSR3::SetConstants(const TemporalSuperSamplingConstants& constants)
    {
        this->constants = constants;
    }

    TemporalSuperSamplingOptimalSettings FidelityFXFSR3::GetOptimalSettings() const
    {
        uint32_t targetWidth = options.outputWidth;
        uint32_t targetHeight = options.outputHeight;
        uint32_t renderWidth = 0;
        uint32_t renderHeight = 0;
        float renderResolutionPercentage = 1.0f;

        if (options.qualityMode < FFX_FSR3_QUALITY_MODE_QUALITY || options.qualityMode > FFX_FSR3_QUALITY_MODE_ULTRA_PERFORMANCE)
        {
            renderResolutionPercentage = std::clamp(options.desiredRenderResolutionPercentage, SuperResolutionMinPercentage, SuperResolutionMaxPercentage);

            renderWidth = uint32_t(renderResolutionPercentage * float(targetWidth));
            renderHeight = uint32_t(renderResolutionPercentage * float(targetHeight));
        }
        else
        {
            FfxFsr3QualityMode fsr3QualityMode = GetFfxFsr3QualityMode(options.qualityMode);

            FfxErrorCode errorCode = ffxFsr3GetRenderResolutionFromQualityMode(
                &renderWidth,
                &renderHeight,
                targetWidth,
                targetHeight,
                fsr3QualityMode);
            FFX_ASSERT(errorCode == FFX_OK);

            float upscaleRatio = ffxFsr3GetUpscaleRatioFromQualityMode(fsr3QualityMode);
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

    uint32 FidelityFXFSR3::GetJitterPhaseCount(uint32 renderWidth, uint32 targetWidth) const
    {
        const int32_t jitterPhaseCount = ffxFsr3GetJitterPhaseCount(int32_t(renderWidth), int32_t(targetWidth));
        return uint32(jitterPhaseCount);
    }

    Vector2f FidelityFXFSR3::GetJitterOffset(uint32 index, uint32 phaseCount) const
    {
        // TODO: Is it safe to cast index and phaseCount to int32_t?
        Vector2f jitterOffset(0.0f, 0.0f);
        FfxErrorCode errorCode = ffxFsr3GetJitterOffset(&jitterOffset.x, &jitterOffset.y, int32_t(index), int32_t(phaseCount));
        FFX_ASSERT(errorCode == FFX_OK);
        return jitterOffset;
    }

    RenderGraphTextureHandle FidelityFXFSR3::Dispatch(
        RenderGraph& renderGraph,
        const SceneView& view,
        const TemporalSuperSamplingDispatchDescription& dispatchDescription)
    {
        RenderBackendDispatchSuperSamplingCallback dispatchCallback = nullptr;
        switch (api)
        {
        case FidelityFXFSR3API::D3D12: dispatchCallback = FidelityFXFSR3DispatchUpscaleD3D12; break;
        case FidelityFXFSR3API::Vulkan: dispatchCallback = FidelityFXFSR3DispatchUpscaleVulkan; break;
        default: std::unreachable(); break;
        }

        void* fsrContext = this;

        RenderGraphTextureDescription outputTextureDesc = RenderGraphTextureDescription::Create2D(
                view.targetWidth,
                view.targetHeight,
                RenderBackendTextureFormat::R16G16B16A16Float,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "FSR3OutputTexture");

        renderGraph.AddPass(
            std::format("FidelityFXFSR3Dispatch (Compute, {}x{} -> {}x{})", renderGraph.GetTextureDescription(dispatchDescription.colorTexture).width, renderGraph.GetTextureDescription(dispatchDescription.colorTexture).height, outputTextureDesc.width, outputTextureDesc.height),
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