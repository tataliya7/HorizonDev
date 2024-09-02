module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

#include <ffx_fsr2.h>
#include <dx12/ffx_fsr2_dx12.h>

module FidelityFX.SuperResolution2:D3D12;

import :Private;

namespace Horizon
{
    bool FidelityFXSuperResolution2DispatchD3D12(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure)
    {
        FidelityFXSuperResolution2* fsr2 = static_cast<FidelityFXSuperResolution2*>(context);
        FidelityFxSuperResolution2State& fsr2State = *fsr2->state;

        FfxFsr2ContextDescription& fsr2ContextDescription = fsr2State.fsr2ContextDescription;
        FfxFsr2Context& fsr2Context = fsr2State.fsr2Context;

        bool isContextValid =
            fsr2State.initialized &&
            fsr2State.fsr2ContextDescription.displaySize.width == fsr2->options.targetWidth &&
            fsr2State.fsr2ContextDescription.displaySize.height == fsr2->options.targetHeight;

        if (!isContextValid)
        {
            RenderBackendDevice device = fsr2->renderBackend->GetNativeDevice();
            ID3D12Device* d3d12Device = static_cast<ID3D12Device*>(device.device);

            uint32 targetWidth = fsr2->options.targetWidth;
            uint32 targetHeight = fsr2->options.targetHeight;

            // only destroy contexts which are live
            if (fsr2ContextDescription.callbacks.scratchBuffer != nullptr)
            {
                ffxFsr2ContextDestroy(&fsr2Context);
                free(fsr2ContextDescription.callbacks.scratchBuffer);
                fsr2ContextDescription.callbacks.scratchBuffer = nullptr;
            }

            const size_t scratchBufferSize = ffxFsr2GetScratchMemorySizeDX12();
            void* scratchBuffer = malloc(scratchBufferSize);
            FfxErrorCode errorCode = ffxFsr2GetInterfaceDX12(&fsr2ContextDescription.callbacks, d3d12Device, scratchBuffer, scratchBufferSize);
            FFX_ASSERT(errorCode == FFX_OK);

            fsr2ContextDescription.device = ffxGetDeviceDX12(d3d12Device);
            fsr2ContextDescription.maxRenderSize.width = targetWidth;
            fsr2ContextDescription.maxRenderSize.height = targetHeight;
            fsr2ContextDescription.displaySize.width = targetWidth;
            fsr2ContextDescription.displaySize.height = targetHeight;

            fsr2ContextDescription.flags |= FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;
            fsr2ContextDescription.flags |= FFX_FSR2_ENABLE_DEPTH_INVERTED;
            //fsr2ContextDescription.flags |= FFX_FSR2_ENABLE_DEPTH_INFINITE;

            //We never use FSR2's own auto-exposure.
            //fsr2ContextDescription.flags |= enableAutoExposure ? FFX_FSR2_ENABLE_AUTO_EXPOSURE : 0;

#if !HORIZON_CONFIGURATION_RELEASE
            //if (device->fsr2EnableDebugCheck)
            {
                fsr2ContextDescription.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
                fsr2ContextDescription.fpMessage = FidelityFXSuperResolution2Message;
            }
#endif

            errorCode = ffxFsr2ContextCreate(&fsr2Context, &fsr2ContextDescription);
            FFX_ASSERT(errorCode == FFX_OK);

            fsr2State.initialized = true;
        }

        FfxFsr2DispatchDescription fsr2DispatchDescription = {};

        fsr2DispatchDescription.output = ffxGetResourceDX12(
            &fsr2Context,
            static_cast<ID3D12Resource*>(output.texture),
            L"FSR2_OutputUpscaledColor",
            FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        fsr2DispatchDescription.color = ffxGetResourceDX12(
            &fsr2Context,
            static_cast<ID3D12Resource*>(color.texture),
            L"FSR2_InputColor",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr2DispatchDescription.depth = ffxGetResourceDX12(
            &fsr2Context,
            static_cast<ID3D12Resource*>(depth.texture),
            L"FSR2_InputDepth",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr2DispatchDescription.motionVectors = ffxGetResourceDX12(
            &fsr2Context,
            static_cast<ID3D12Resource*>(motionVectors.texture),
            L"FSR2_InputMotionVectors",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        if (false)
        {
            fsr2DispatchDescription.exposure = ffxGetResourceDX12(
                &fsr2Context,
                nullptr,
                L"FSR2_InputExposure");
        }
        else
        {
            fsr2DispatchDescription.exposure = ffxGetResourceDX12(
                &fsr2Context,
                static_cast<ID3D12Resource*>(exposure.texture),
                L"FSR2_InputExposure");
        }

        if (true)
        {
            fsr2DispatchDescription.reactive = ffxGetResourceDX12(
                &fsr2Context,
                nullptr,
                L"FSR2_EmptyInputReactiveMap");
        }
        else
        {
            // TODO
        }

        if (true)
        {
            fsr2DispatchDescription.transparencyAndComposition = ffxGetResourceDX12(
                &fsr2Context,
                nullptr,
                L"FSR2_EmptyTransparencyAndCompositionMap");
        }
        else
        {
            // TODO
        }

        const TemporalSuperSamplingConstants& constants = fsr2->constants;
        fsr2DispatchDescription.commandList = ffxGetCommandListDX12(static_cast<ID3D12CommandList*>(commandList));
        fsr2DispatchDescription.jitterOffset.x = constants.jitterOffsetX;
        fsr2DispatchDescription.jitterOffset.y = constants.jitterOffsetY;
        fsr2DispatchDescription.motionVectorScale.x = constants.motionVectorScaleX;
        fsr2DispatchDescription.motionVectorScale.y = constants.motionVectorScaleY;
        fsr2DispatchDescription.reset = constants.reset;
        fsr2DispatchDescription.enableSharpening = constants.sharpness > 0.0f;
        fsr2DispatchDescription.sharpness = constants.sharpness;
        fsr2DispatchDescription.frameTimeDelta = constants.deltaTime; // 'frameTimeDelta' is expressed in milliseconds
        fsr2DispatchDescription.preExposure = constants.preExposure;
        fsr2DispatchDescription.renderSize.width = constants.renderWidth;
        fsr2DispatchDescription.renderSize.height = constants.renderHeight;
        fsr2DispatchDescription.cameraFar = constants.cameraFarClippingPlane;
        fsr2DispatchDescription.cameraNear = constants.cameraNearClippingPlane;
        fsr2DispatchDescription.cameraFovAngleVertical = constants.cameraFovAngleVertical;
        fsr2DispatchDescription.viewSpaceToMetersFactor = 1.0f;

        if (fsr2ContextDescription.flags & FFX_FSR2_ENABLE_DEPTH_INVERTED)
        {
            std::swap(fsr2DispatchDescription.cameraFar, fsr2DispatchDescription.cameraNear);
        }

        FfxErrorCode errorCode = ffxFsr2ContextDispatch(&fsr2Context, &fsr2DispatchDescription);
        FFX_ASSERT(errorCode == FFX_OK);

        return (errorCode == FFX_OK);
    }
}