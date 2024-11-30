module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

#include <FidelityFX/host/ffx_fsr2.h>
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>

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
            fsr2State.fsr2ContextDescription.displaySize.width == fsr2->options.outputWidth &&
            fsr2State.fsr2ContextDescription.displaySize.height == fsr2->options.outputHeight;

        if (!isContextValid)
        {
            RenderBackendDevice device = fsr2->renderBackend->GetNativeDevice();
            ID3D12Device* d3d12Device = static_cast<ID3D12Device*>(device.device);
            FfxDevice ffxDevice = ffxGetDeviceDX12(d3d12Device);

            uint32 targetWidth = fsr2->options.outputWidth;
            uint32 targetHeight = fsr2->options.outputHeight;

            // only destroy contexts which are live
            if (fsr2ContextDescription.backendInterface.scratchBuffer != nullptr)
            {
                ffxFsr2ContextDestroy(&fsr2Context);
                free(fsr2ContextDescription.backendInterface.scratchBuffer);
                fsr2ContextDescription.backendInterface.scratchBuffer = nullptr;
            }

            const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(1);
            void* scratchBuffer = malloc(scratchBufferSize);
            memset(scratchBuffer, 0, scratchBufferSize);

            FfxErrorCode errorCode = ffxGetInterfaceDX12(&fsr2ContextDescription.backendInterface, ffxDevice, scratchBuffer, scratchBufferSize, 1);
            FFX_ASSERT(errorCode == FFX_OK);

            fsr2ContextDescription.maxRenderSize.width = targetWidth;
            fsr2ContextDescription.maxRenderSize.height = targetHeight;
            fsr2ContextDescription.displaySize.width = targetWidth;
            fsr2ContextDescription.displaySize.height = targetHeight;

            fsr2ContextDescription.flags |= FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;
            fsr2ContextDescription.flags |= FFX_FSR2_ENABLE_DEPTH_INVERTED;
#if HORIZON_EXPERIMENTAL_INFINITE_PERSPECTIVE
            fsr2ContextDescription.flags |= FFX_FSR2_ENABLE_DEPTH_INFINITE;
#endif

            // We never use FSR2's own auto-exposure.
            // fsr2ContextDescription.flags |= enableAutoExposure ? FFX_FSR2_ENABLE_AUTO_EXPOSURE : 0;

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

        ID3D12Resource* outputTextureDX12 = static_cast<ID3D12Resource*>(output.texture);
        FfxResourceDescription outputTextureDescription = ffxGetResourceDescriptionDX12(outputTextureDX12);
        fsr2DispatchDescription.output = ffxGetResourceDX12(
            outputTextureDX12,
            outputTextureDescription,
            L"FSR2_OutputUpscaledColor",
            FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        ID3D12Resource* colorTextureDX12 = static_cast<ID3D12Resource*>(color.texture);
        FfxResourceDescription colorTextureDescription = ffxGetResourceDescriptionDX12(colorTextureDX12);
        fsr2DispatchDescription.color = ffxGetResourceDX12(
            colorTextureDX12,
            colorTextureDescription,
            L"FSR2_InputColor",
            FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

        ID3D12Resource* depthTextureDX12 = static_cast<ID3D12Resource*>(depth.texture);
        FfxResourceDescription depthTextureDescription = ffxGetResourceDescriptionDX12(depthTextureDX12);
        fsr2DispatchDescription.depth = ffxGetResourceDX12(
            depthTextureDX12,
            depthTextureDescription,
            L"FSR2_InputDepth",
            FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

        ID3D12Resource* motionVectorTextureDX12 = static_cast<ID3D12Resource*>(motionVectors.texture);
        FfxResourceDescription motionVectorTextureDescription = ffxGetResourceDescriptionDX12(motionVectorTextureDX12);
        fsr2DispatchDescription.motionVectors = ffxGetResourceDX12(
            motionVectorTextureDX12,
            motionVectorTextureDescription,
            L"FSR2_InputMotionVectors",
            FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

        if (false)
        {
            fsr2DispatchDescription.exposure = ffxGetResourceDX12(
                nullptr,
                ffxGetResourceDescriptionDX12(nullptr),
                L"FSR2_InputExposure");
        }
        else
        {
            ID3D12Resource* exposureTextureDX12 = static_cast<ID3D12Resource*>(exposure.texture);
            FfxResourceDescription exposureTextureDescription = ffxGetResourceDescriptionDX12(exposureTextureDX12);
            fsr2DispatchDescription.exposure = ffxGetResourceDX12(
                exposureTextureDX12,
                exposureTextureDescription,
                L"FSR2_InputExposure",
                FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        }

        if (true)
        {
            fsr2DispatchDescription.reactive = ffxGetResourceDX12(
                nullptr,
                ffxGetResourceDescriptionDX12(nullptr),
                L"FSR2_EmptyInputReactiveMap");
        }
        else
        {
            // TODO
        }

        if (true)
        {
            fsr2DispatchDescription.transparencyAndComposition = ffxGetResourceDX12(
                nullptr,
                ffxGetResourceDescriptionDX12(nullptr),
                L"FSR2_EmptyTransparencyAndCompositionMap");
        }
        else
        {
            // TODO
        }

        const TemporalSuperSamplingConstants& constants = fsr2->constants;
        fsr2DispatchDescription.commandList = ffxGetCommandListDX12(static_cast<ID3D12CommandList*>(commandList));
        fsr2DispatchDescription.jitterOffset.x = constants.jitterOffset.x;
        fsr2DispatchDescription.jitterOffset.y = constants.jitterOffset.y;
        fsr2DispatchDescription.motionVectorScale.x = constants.motionVectorScale.x * float(constants.renderWidth);
        fsr2DispatchDescription.motionVectorScale.y = constants.motionVectorScale.y * float(constants.renderHeight);
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