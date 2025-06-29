module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

#include <FidelityFX/host/ffx_fsr3.h>
#include <FidelityFX/host/backends/vk/ffx_vk.h>

module FidelityFX.FSR3:Vulkan;

import :Private;

namespace Horizon
{
    bool FidelityFXFSR3DispatchUpscaleVulkan(
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

        bool isContextValid = fsr3State.initialized &&
            fsr3State.fsr3ContextDescription.displaySize.width == fsr3->options.outputWidth &&
            fsr3State.fsr3ContextDescription.displaySize.height == fsr3->options.outputHeight;

        if (!isContextValid)
        {
            RenderBackendDeviceContext device = fsr3->renderBackend->GetNativeDevice();

            VkDevice vkDevice = static_cast<VkDevice>(device.device);
            VkPhysicalDevice vkPhysicalDevice = static_cast<VkPhysicalDevice>(device.physicalDevice);

            VkDeviceContext ffxDeviceContext =
            {
                .vkDevice = vkDevice,
                .vkPhysicalDevice = vkPhysicalDevice,
                .vkDeviceProcAddr = static_cast<PFN_vkGetDeviceProcAddr>(device.vkGetDeviceProcAddr)
            };
            FfxDevice ffxDevice = ffxGetDeviceVK(&ffxDeviceContext);

            uint32 targetWidth = fsr3->options.outputWidth;
            uint32 targetHeight = fsr3->options.outputHeight;

            // Only destroy contexts which are live
            // if (fsr3ContextDescription.backendInterfaceUpscaling.scratchBuffer != nullptr)
            // {
            //     ffxFsr3ContextDestroy(fsr3Context);
            //     free(fsr3ContextDescription.backendInterfaceUpscaling.scratchBuffer);
            //     fsr3ContextDescription.backendInterfaceUpscaling.scratchBuffer = nullptr;
            // }

            // @todo We must fetch all three interfaces to avoid FFX bugs.
            {
                const size_t scratchBufferSize = ffxGetScratchMemorySizeVK(vkPhysicalDevice, 1);
                void* scratchBuffer = calloc(scratchBufferSize, 1);

                FfxErrorCode errorCode = ffxGetInterfaceVK(&fsr3ContextDescription.backendInterfaceSharedResources, ffxDevice, scratchBuffer, scratchBufferSize, 1);
                FFX_ASSERT(errorCode == FFX_OK);
            }
            {
                const size_t scratchBufferSize = ffxGetScratchMemorySizeVK(vkPhysicalDevice, 1);
                void* scratchBuffer = calloc(scratchBufferSize, 1);

                FfxErrorCode errorCode = ffxGetInterfaceVK(&fsr3ContextDescription.backendInterfaceUpscaling, ffxDevice, scratchBuffer, scratchBufferSize, 1);
                FFX_ASSERT(errorCode == FFX_OK);
            }
            {
                const size_t scratchBufferSize = ffxGetScratchMemorySizeVK(vkPhysicalDevice, 2);
                void* scratchBuffer = calloc(scratchBufferSize, 1);

                FfxErrorCode errorCode = ffxGetInterfaceVK(&fsr3ContextDescription.backendInterfaceFrameInterpolation, ffxDevice, scratchBuffer, scratchBufferSize, 1);
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
            fsr3ContextDescription.flags |= FFX_FSR2_ENABLE_DEPTH_INFINITE;
#endif
            // We never use FSR2's own auto-exposure.
            //fsr3ContextDescription.flags |= enableAutoExposure ? FFX_FSR2_ENABLE_AUTO_EXPOSURE : 0;

#if !HORIZON_CONFIGURATION_RELEASE
            // if (device->fsr3EnableDebugCheck)
            {
                fsr3ContextDescription.flags |= FFX_FSR3_ENABLE_DEBUG_CHECKING;
                fsr3ContextDescription.fpMessage = FidelityFXFSR3Message;
            }
#endif

            fsr3ContextDescription.flags |= FFX_FSR3_ENABLE_UPSCALING_ONLY;

            bool upscalingOnly                      = (fsr3ContextDescription.flags & FFX_FSR3_ENABLE_UPSCALING_ONLY) != 0;

            // @todo
            fsr3ContextDescription.backBufferFormat = FFX_SURFACE_FORMAT_R10G10B10A2_UNORM;

            //const uint64_t memoryUsageBefore = getMemoryUsageSnapshot(device->GetPhysicalDeviceHandle());
            FfxErrorCode errorCode = ffxFsr3ContextCreate(fsr3Context, &fsr3ContextDescription);
            FFX_ASSERT(errorCode == FFX_OK);

            fsr3State.initialized = true;
        }

        FfxFsr3DispatchUpscaleDescription fsr3DispatchDescription = {};

        fsr3DispatchDescription.upscaleOutput = ffxGetResourceVK(
            output.texture,
            ffxGetImageResourceDescriptionVK(static_cast<VkImage>(output.texture), *static_cast<VkImageCreateInfo*>(output.info)),
            L"FSR2_OutputUpscaledColor",
            FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        fsr3DispatchDescription.color = ffxGetResourceVK(
            color.texture,
            ffxGetImageResourceDescriptionVK(static_cast<VkImage>(color.texture), *static_cast<VkImageCreateInfo*>(color.info)),
            L"FSR2_InputColor",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr3DispatchDescription.depth = ffxGetResourceVK(
            depth.texture,
            ffxGetImageResourceDescriptionVK(static_cast<VkImage>(depth.texture), *static_cast<VkImageCreateInfo*>(depth.info)),
            L"FSR2_InputDepth",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr3DispatchDescription.motionVectors = ffxGetResourceVK(
            motionVectors.texture,
            ffxGetImageResourceDescriptionVK(static_cast<VkImage>(motionVectors.texture), *static_cast<VkImageCreateInfo*>(motionVectors.info)),
            L"FSR2_InputMotionVectors",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        if (false)
        {
            fsr3DispatchDescription.exposure = ffxGetResourceVK(
                nullptr,
                FfxResourceDescription(),
                L"FSR2_InputExposure");
        }
        else
        {
            fsr3DispatchDescription.exposure = ffxGetResourceVK(
                exposure.texture,
                ffxGetImageResourceDescriptionVK(static_cast<VkImage>(exposure.texture), *static_cast<VkImageCreateInfo*>(exposure.info)),
                L"FSR2_InputExposure",
                FFX_RESOURCE_STATE_COMPUTE_READ);
        }

        if (true)
        {
            fsr3DispatchDescription.reactive = ffxGetResourceVK(
                nullptr,
                FfxResourceDescription(),
                L"FSR2_EmptyInputReactiveMap");
        }
        else
        {
            // TODO
        }

        if (true)
        {
            fsr3DispatchDescription.transparencyAndComposition = ffxGetResourceVK(
                nullptr,
                FfxResourceDescription(),
                L"FSR2_EmptyTransparencyAndCompositionMap");
        }
        else
        {
            // TODO
        }

        const TemporalSuperSamplingConstants& constants = fsr3->constants;
        fsr3DispatchDescription.commandList = ffxGetCommandListVK(static_cast<VkCommandBuffer>(commandList));
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