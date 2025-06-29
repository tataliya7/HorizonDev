module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

#include <FidelityFX/host/ffx_fsr3.h>
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>

module FidelityFX.FSR3:D3D12;

import :Private;

namespace Horizon
{
    bool FidelityFXFSR3DispatchUpscaleD3D12(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure)
    {
        FidelityFXFSR3* fsr3 = static_cast<FidelityFXFSR3*>(context);
        FidelityFXFSR3State& fsr3State = fsr3->state;

        FfxFsr3ContextDescription& fsr3ContextDescription = fsr3State.fsr3ContextDescription;
        FfxFsr3Context* fsr3Context = fsr3State.fsr3Context;

        bool isContextValid =
            fsr3State.initialized &&
            fsr3State.fsr3ContextDescription.displaySize.width == fsr3->options.outputWidth &&
            fsr3State.fsr3ContextDescription.displaySize.height == fsr3->options.outputHeight;

        if (!isContextValid)
        {
            RenderBackendDeviceContext device = fsr3->renderBackend->GetNativeDevice();
            ID3D12Device* d3d12Device = static_cast<ID3D12Device*>(device.device);
            FfxDevice ffxDevice = ffxGetDeviceDX12(d3d12Device);

            uint32 targetWidth = fsr3->options.outputWidth;
            uint32 targetHeight = fsr3->options.outputHeight;

            // only destroy contexts which are live
            // if (fsr3ContextDescription.backendInterfaceUpscaling.scratchBuffer != nullptr)
            // {
            //     ffxFsr3ContextDestroy(fsr3Context);
            //     free(fsr3ContextDescription.backendInterfaceUpscaling.scratchBuffer);
            //     fsr3ContextDescription.backendInterfaceUpscaling.scratchBuffer = nullptr;
            // }

            {
                const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(1);
                void* scratchBuffer = calloc(scratchBufferSize, 1);

                FfxErrorCode errorCode = ffxGetInterfaceDX12(&fsr3ContextDescription.backendInterfaceSharedResources, ffxDevice, scratchBuffer, scratchBufferSize, 1);
                FFX_ASSERT(errorCode == FFX_OK);
            }
            {
                const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(1);
                void* scratchBuffer = calloc(scratchBufferSize, 1);

                FfxErrorCode errorCode = ffxGetInterfaceDX12(&fsr3ContextDescription.backendInterfaceUpscaling, ffxDevice, scratchBuffer, scratchBufferSize, 1);
                FFX_ASSERT(errorCode == FFX_OK);
            }
            {
                const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(2);
                void* scratchBuffer = calloc(scratchBufferSize, 1);

                FfxErrorCode errorCode = ffxGetInterfaceDX12(&fsr3ContextDescription.backendInterfaceFrameInterpolation, ffxDevice, scratchBuffer, scratchBufferSize, 1);
                FFX_ASSERT(errorCode == FFX_OK);
            }

            fsr3ContextDescription.maxRenderSize.width = targetWidth;
            fsr3ContextDescription.maxRenderSize.height = targetHeight;
            fsr3ContextDescription.maxUpscaleSize.width = targetWidth;
            fsr3ContextDescription.maxUpscaleSize.height = targetHeight;
            fsr3ContextDescription.displaySize.width = targetWidth;
            fsr3ContextDescription.displaySize.height = targetHeight;

            fsr3ContextDescription.flags |= FFX_FSR3_ENABLE_HIGH_DYNAMIC_RANGE;
            fsr3ContextDescription.flags |= FFX_FSR3_ENABLE_DEPTH_INVERTED;
#if HORIZON_EXPERIMENTAL_INFINITE_PERSPECTIVE
            fsr3ContextDescription.flags |= FFX_FSR3_ENABLE_DEPTH_INFINITE;
#endif

            // We never use FSR3's own auto-exposure.
            // fsr3ContextDescription.flags |= enableAutoExposure ? FFX_FSR3_ENABLE_AUTO_EXPOSURE : 0;

#if !HORIZON_CONFIGURATION_RELEASE
            //if (device->fsr3EnableDebugCheck)
            {
                fsr3ContextDescription.flags |= FFX_FSR3_ENABLE_DEBUG_CHECKING;
                fsr3ContextDescription.fpMessage = FidelityFXFSR3Message;
            }
#endif

            fsr3ContextDescription.flags |= FFX_FSR3_ENABLE_UPSCALING_ONLY;

            // @todo
            fsr3ContextDescription.backBufferFormat = FFX_SURFACE_FORMAT_R10G10B10A2_UNORM;

            FfxErrorCode errorCode = ffxFsr3ContextCreate(fsr3Context, &fsr3ContextDescription);
            FFX_ASSERT(errorCode == FFX_OK);

            fsr3State.initialized = true;
        }

        FfxFsr3DispatchUpscaleDescription fsr3DispatchDescription = {};

        ID3D12Resource* outputTextureDX12 = static_cast<ID3D12Resource*>(output.texture);
        FfxResourceDescription outputTextureDescription = ffxGetResourceDescriptionDX12(outputTextureDX12);
        fsr3DispatchDescription.upscaleOutput = ffxGetResourceDX12(
            outputTextureDX12,
            outputTextureDescription,
            L"FSR3_OutputUpscaledColor",
            FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        ID3D12Resource* colorTextureDX12 = static_cast<ID3D12Resource*>(color.texture);
        FfxResourceDescription colorTextureDescription = ffxGetResourceDescriptionDX12(colorTextureDX12);
        fsr3DispatchDescription.color = ffxGetResourceDX12(
            colorTextureDX12,
            colorTextureDescription,
            L"FSR3_InputColor",
            FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

        ID3D12Resource* depthTextureDX12 = static_cast<ID3D12Resource*>(depth.texture);
        FfxResourceDescription depthTextureDescription = ffxGetResourceDescriptionDX12(depthTextureDX12);
        fsr3DispatchDescription.depth = ffxGetResourceDX12(
            depthTextureDX12,
            depthTextureDescription,
            L"FSR3_InputDepth",
            FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

        ID3D12Resource* motionVectorTextureDX12 = static_cast<ID3D12Resource*>(motionVectors.texture);
        FfxResourceDescription motionVectorTextureDescription = ffxGetResourceDescriptionDX12(motionVectorTextureDX12);
        fsr3DispatchDescription.motionVectors = ffxGetResourceDX12(
            motionVectorTextureDX12,
            motionVectorTextureDescription,
            L"FSR3_InputMotionVectors",
            FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);

        if (false)
        {
            fsr3DispatchDescription.exposure = ffxGetResourceDX12(
                nullptr,
                ffxGetResourceDescriptionDX12(nullptr),
                L"FSR3_InputExposure");
        }
        else
        {
            ID3D12Resource* exposureTextureDX12 = static_cast<ID3D12Resource*>(exposure.texture);
            FfxResourceDescription exposureTextureDescription = ffxGetResourceDescriptionDX12(exposureTextureDX12);
            fsr3DispatchDescription.exposure = ffxGetResourceDX12(
                exposureTextureDX12,
                exposureTextureDescription,
                L"FSR3_InputExposure",
                FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        }

        if (true)
        {
            fsr3DispatchDescription.reactive = ffxGetResourceDX12(
                nullptr,
                ffxGetResourceDescriptionDX12(nullptr),
                L"FSR3_EmptyInputReactiveMap");
        }
        else
        {
            // TODO
        }

        if (true)
        {
            fsr3DispatchDescription.transparencyAndComposition = ffxGetResourceDX12(
                nullptr,
                ffxGetResourceDescriptionDX12(nullptr),
                L"FSR3_EmptyTransparencyAndCompositionMap");
        }
        else
        {
            // TODO
        }

        const TemporalSuperSamplingConstants& constants = fsr3->constants;
        fsr3DispatchDescription.commandList = ffxGetCommandListDX12(static_cast<ID3D12CommandList*>(commandList));
        fsr3DispatchDescription.jitterOffset.x = constants.jitterOffset.x;
        fsr3DispatchDescription.jitterOffset.y = constants.jitterOffset.y;
        fsr3DispatchDescription.motionVectorScale.x = constants.motionVectorScale.x * static_cast<float>(constants.renderWidth);
        fsr3DispatchDescription.motionVectorScale.y = constants.motionVectorScale.y * static_cast<float>(constants.renderHeight);
        fsr3DispatchDescription.renderSize.width = constants.renderWidth;
        fsr3DispatchDescription.renderSize.height = constants.renderHeight;
        fsr3DispatchDescription.upscaleSize.width = output.width;
        fsr3DispatchDescription.upscaleSize.height = output.height;
        fsr3DispatchDescription.enableSharpening = constants.sharpness > 0.0f;
        fsr3DispatchDescription.sharpness = constants.sharpness;
        fsr3DispatchDescription.frameTimeDelta = constants.deltaTime; // 'frameTimeDelta' is expressed in milliseconds
        fsr3DispatchDescription.preExposure = constants.preExposure;
        fsr3DispatchDescription.reset = constants.reset;
        fsr3DispatchDescription.cameraFar = constants.cameraFarClippingPlane;
        fsr3DispatchDescription.cameraNear = constants.cameraNearClippingPlane;
        fsr3DispatchDescription.cameraFovAngleVertical = constants.cameraFovAngleVertical;
        fsr3DispatchDescription.viewSpaceToMetersFactor = 1.0f;
        fsr3DispatchDescription.flags = 0;
        fsr3DispatchDescription.frameID = constants.frameIndex;

        if (fsr3ContextDescription.flags & FFX_FSR3_ENABLE_DEPTH_INVERTED)
        {
            std::swap(fsr3DispatchDescription.cameraFar, fsr3DispatchDescription.cameraNear);
        }

        FfxErrorCode errorCode = ffxFsr3ContextDispatchUpscale(fsr3Context, &fsr3DispatchDescription);
        FFX_ASSERT(errorCode == FFX_OK);

        return (errorCode == FFX_OK);
    }
}