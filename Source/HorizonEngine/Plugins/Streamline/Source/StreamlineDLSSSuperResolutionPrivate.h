#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>

namespace Horizon
{
    class StreamlineDLSSSuperResolution : public TemporalSuperSamplingInterface
    {
    public:
        StreamlineDLSSSuperResolution(RenderBackend* renderBackend);
        virtual ~StreamlineDLSSSuperResolution();
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

        friend bool StreamlineDLSSSuperResolutionDispatchD3D12(
            void* commandList,
            void* context,
            const RenderBackendTextureResource& output,
            const RenderBackendTextureResource& color,
            const RenderBackendTextureResource& depth,
            const RenderBackendTextureResource& motionVectors,
            const RenderBackendTextureResource& exposure);

        friend bool StreamlineDLSSSuperResolutionDispatchVulkan(
            void* commandList,
            void* context,
            const RenderBackendTextureResource& output,
            const RenderBackendTextureResource& color,
            const RenderBackendTextureResource& depth,
            const RenderBackendTextureResource& motionVectors,
            const RenderBackendTextureResource& exposure);

        RenderBackend* renderBackend;
        //DLSSSuperResolutionAPI api;
        sl::DLSSOptions options;
        sl::FrameToken* frameToken = nullptr;
        TemporalSuperSamplingConstants constants;
    };

    bool StreamlineDLSSSuperResolutionDispatchD3D12(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure);

    bool StreamlineDLSSSuperResolutionDispatchVulkan(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure);
}