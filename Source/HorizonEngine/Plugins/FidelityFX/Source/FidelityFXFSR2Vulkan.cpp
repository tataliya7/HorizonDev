module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

#include <FidelityFX/host/ffx_fsr2.h>
#include <FidelityFX/host/backends/vk/ffx_vk.h>

module FidelityFX.FSR2:Vulkan;

import :Private;

namespace Horizon
{
    bool FidelityFXSuperResolution2DispatchVulkan(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure)
    {
        FidelityFXFSR2* fsr2 = static_cast<FidelityFXFSR2*>(context);
        FidelityFXSuperResolution2State& fsr2State = *fsr2->state;

        FfxFsr2ContextDescription& fsr2ContextDescription = fsr2State.fsr2ContextDescription;
        FfxFsr2Context& fsr2Context = fsr2State.fsr2Context;

        bool isContextValid = fsr2State.initialized &&
            fsr2State.fsr2ContextDescription.displaySize.width == fsr2->options.outputWidth &&
            fsr2State.fsr2ContextDescription.displaySize.height == fsr2->options.outputHeight;

        if (!isContextValid)
        {
            RenderBackendDeviceContext device = fsr2->renderBackend->GetNativeDevice();

            VkDevice vkDevice = static_cast<VkDevice>(device.device);
            VkPhysicalDevice vkPhysicalDevice = static_cast<VkPhysicalDevice>(device.physicalDevice);

            VkDeviceContext ffxDeviceContext =
            {
                .vkDevice = vkDevice,
                .vkPhysicalDevice = vkPhysicalDevice,
                .vkDeviceProcAddr = static_cast<PFN_vkGetDeviceProcAddr>(device.vkGetDeviceProcAddr)
            };
            FfxDevice ffxDevice = ffxGetDeviceVK(&ffxDeviceContext);

            uint32 targetWidth = fsr2->options.outputWidth;
            uint32 targetHeight = fsr2->options.outputHeight;

            // Only destroy contexts which are live
            if (fsr2ContextDescription.backendInterface.scratchBuffer != nullptr)
            {
                ffxFsr2ContextDestroy(&fsr2Context);
                free(fsr2ContextDescription.backendInterface.scratchBuffer);
                fsr2ContextDescription.backendInterface.scratchBuffer = nullptr;
            }

            const size_t scratchBufferSize = ffxGetScratchMemorySizeVK(vkPhysicalDevice, 1);
            void* scratchBuffer = malloc(scratchBufferSize);
            memset(scratchBuffer, 0, scratchBufferSize);

            FfxErrorCode errorCode = ffxGetInterfaceVK(&fsr2ContextDescription.backendInterface, ffxDevice, scratchBuffer, scratchBufferSize, 1);
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
            //fsr2ContextDescription.flags |= enableAutoExposure ? FFX_FSR2_ENABLE_AUTO_EXPOSURE : 0;

#if !HORIZON_CONFIGURATION_RELEASE
            // if (device->fsr2EnableDebugCheck)
            {
                fsr2ContextDescription.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
                fsr2ContextDescription.fpMessage = FidelityFXSuperResolution2Message;
            }
#endif

            //const uint64_t memoryUsageBefore = getMemoryUsageSnapshot(device->GetPhysicalDeviceHandle());
            errorCode = ffxFsr2ContextCreate(&fsr2Context, &fsr2ContextDescription);
            FFX_ASSERT(errorCode == FFX_OK);

            fsr2State.initialized = true;
        }

        FfxFsr2DispatchDescription fsr2DispatchDescription = {};

        fsr2DispatchDescription.output = ffxGetResourceVK(
            output.texture,
            ffxGetImageResourceDescriptionVK(static_cast<VkImage>(output.texture), *static_cast<VkImageCreateInfo*>(output.info)),
            L"FSR2_OutputUpscaledColor",
            FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        fsr2DispatchDescription.color = ffxGetResourceVK(
            color.texture,
            ffxGetImageResourceDescriptionVK(static_cast<VkImage>(color.texture), *static_cast<VkImageCreateInfo*>(color.info)),
            L"FSR2_InputColor",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr2DispatchDescription.depth = ffxGetResourceVK(
            depth.texture,
            ffxGetImageResourceDescriptionVK(static_cast<VkImage>(depth.texture), *static_cast<VkImageCreateInfo*>(depth.info)),
            L"FSR2_InputDepth",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr2DispatchDescription.motionVectors = ffxGetResourceVK(
            motionVectors.texture,
            ffxGetImageResourceDescriptionVK(static_cast<VkImage>(motionVectors.texture), *static_cast<VkImageCreateInfo*>(motionVectors.info)),
            L"FSR2_InputMotionVectors",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        if (false)
        {
            fsr2DispatchDescription.exposure = ffxGetResourceVK(
                nullptr,
                FfxResourceDescription(),
                L"FSR2_InputExposure");
        }
        else
        {
            fsr2DispatchDescription.exposure = ffxGetResourceVK(
                exposure.texture,
                ffxGetImageResourceDescriptionVK(static_cast<VkImage>(exposure.texture), *static_cast<VkImageCreateInfo*>(exposure.info)),
                L"FSR2_InputExposure",
                FFX_RESOURCE_STATE_COMPUTE_READ);
        }

        if (true)
        {
            fsr2DispatchDescription.reactive = ffxGetResourceVK(
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
            fsr2DispatchDescription.transparencyAndComposition = ffxGetResourceVK(
                nullptr,
                FfxResourceDescription(),
                L"FSR2_EmptyTransparencyAndCompositionMap");
        }
        else
        {
            // TODO
        }

        const TemporalSuperSamplingConstants& constants = fsr2->constants;
        fsr2DispatchDescription.commandList = ffxGetCommandListVK(static_cast<VkCommandBuffer>(commandList));
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