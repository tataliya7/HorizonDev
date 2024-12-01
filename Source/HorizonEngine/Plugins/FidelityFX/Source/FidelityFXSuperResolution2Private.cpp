module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

#include <FidelityFX/host/ffx_fsr2.h>

module FidelityFX.SuperResolution2:Private;

import FidelityFX.SuperResolution2;

namespace Horizon
{
    struct FidelityFXSuperResolution2State
    {
        FfxFsr2ContextDescription fsr2ContextDescription;
        FfxFsr2Context fsr2Context;
        bool initialized;
        //uint32 viewportID;
    };

    class FidelityFXSuperResolution2 : public TemporalSuperSamplingInterface
    {
    public:
        FidelityFXSuperResolution2(RenderBackend* renderBackend);
        virtual ~FidelityFXSuperResolution2();
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
        FidelityFXSuperResolution2State* state;
        TemporalSuperSamplingOptions options;
        TemporalSuperSamplingConstants constants;
    };

    void FidelityFXSuperResolution2Message(FfxMsgType type, const wchar_t* message)
    {
        if (type == FFX_MESSAGE_TYPE_ERROR)
        {
            Horizon::LogError(Horizon::GLogger, std::format(L"FSR2_API_DEBUG_ERROR: {}", message));
        }
        else if (type == FFX_MESSAGE_TYPE_WARNING)
        {
            Horizon::LogWarning(Horizon::GLogger, std::format(L"FSR2_API_DEBUG_WARNING: {}", message));
        }
    }

    bool FidelityFXSuperResolution2DispatchD3D12(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure);

    bool FidelityFXSuperResolution2DispatchVulkan(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure);
}