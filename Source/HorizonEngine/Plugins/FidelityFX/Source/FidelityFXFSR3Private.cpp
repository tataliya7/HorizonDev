module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

#include <FidelityFX/host/ffx_fsr3.h>

module FidelityFX.FSR3:Private;

import FidelityFX.FSR3;

namespace Horizon
{
    struct FidelityFXFSR3State
    {
        FfxFsr3ContextDescription fsr3ContextDescription;
        FfxFsr3Context* fsr3Context;
        bool initialized;
        //uint32 viewportID;
    };

    class FidelityFXFSR3 : public TemporalSuperSamplingInterface
    {
    public:
        FidelityFXFSR3(RenderBackend* renderBackend);
        virtual ~FidelityFXFSR3();
        TemporalSuperSamplingConstants GetConstants() const
        {
            return constants;
        }
        void SetOptions(const TemporalSuperSamplingOptions& options) override;
        void SetConstants(const TemporalSuperSamplingConstants& constants) override;
        TemporalSuperSamplingOptimalSettings GetOptimalSettings() const override;
        uint32 GetJitterPhaseCount(uint32 renderWidth, uint32 targetWidth) const override;
        Vector2f GetJitterOffset(uint32 index, uint32 phaseCount) const override;
        RenderGraphTextureHandle Dispatch(RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchDescription& dispatchDescription) override;

    private:

        friend bool FidelityFXFSR3DispatchUpscaleD3D12(
            void* commandList,
            void* context,
            const RenderBackendTextureResource& output,
            const RenderBackendTextureResource& color,
            const RenderBackendTextureResource& depth,
            const RenderBackendTextureResource& motionVectors,
            const RenderBackendTextureResource& exposure);

        friend bool FidelityFXFSR3DispatchUpscaleVulkan(
            void* commandList,
            void* context,
            const RenderBackendTextureResource& output,
            const RenderBackendTextureResource& color,
            const RenderBackendTextureResource& depth,
            const RenderBackendTextureResource& motionVectors,
            const RenderBackendTextureResource& exposure);

        RenderBackend* renderBackend;
        FidelityFXFSR3API api;
        FidelityFXFSR3State state;
        TemporalSuperSamplingOptions options;
        TemporalSuperSamplingConstants constants;
    };

    void FidelityFXFSR3Message(FfxMsgType type, const wchar_t* message)
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

    bool FidelityFXFSR3DispatchUpscaleD3D12(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure);

    bool FidelityFXFSR3DispatchUpscaleVulkan(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure);
}