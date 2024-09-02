module;

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

#include <ffx_fsr2.h>
#include <vk/ffx_fsr2_vk.h>

module FidelityFX.SuperResolution2:Vulkan;

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

            VkDevice vkDevice = static_cast<VkDevice>(device.device);
            VkPhysicalDevice vkPhysicalDevice = static_cast<VkPhysicalDevice>(device.physicalDevice);

            uint32 targetWidth = fsr2->options.targetWidth;
            uint32 targetHeight = fsr2->options.targetHeight;

            // Only destroy contexts which are live
            if (fsr2ContextDescription.callbacks.scratchBuffer != nullptr)
            {
                ffxFsr2ContextDestroy(&fsr2Context);
                free(fsr2ContextDescription.callbacks.scratchBuffer);
                fsr2ContextDescription.callbacks.scratchBuffer = nullptr;
            }

            size_t scratchBufferSize = ffxFsr2GetScratchMemorySizeVK(vkPhysicalDevice);
            void* scratchBuffer = malloc(scratchBufferSize);
            FfxErrorCode errorCode = ffxFsr2GetInterfaceVK(&fsr2ContextDescription.callbacks, scratchBuffer, scratchBufferSize, vkPhysicalDevice, vkGetDeviceProcAddr);
            FFX_ASSERT(errorCode == FFX_OK);

            fsr2ContextDescription.device = ffxGetDeviceVK(vkDevice);
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

        fsr2DispatchDescription.output = ffxGetTextureResourceVK(
            &fsr2Context,
            static_cast<VkImage>(output.texture),
            static_cast<VkImageView>(output.view),
            static_cast<uint32_t>(output.width),
            static_cast<uint32_t>(output.height),
            static_cast<VkFormat>(output.format),
            L"FSR2_OutputUpscaledColor",
            FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        fsr2DispatchDescription.color = ffxGetTextureResourceVK(
            &fsr2Context,
            static_cast<VkImage>(color.texture),
            static_cast<VkImageView>(color.view),
            static_cast<uint32_t>(color.width),
            static_cast<uint32_t>(color.height),
            static_cast<VkFormat>(color.format),
            L"FSR2_InputColor",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr2DispatchDescription.depth = ffxGetTextureResourceVK(
            &fsr2Context,
            static_cast<VkImage>(depth.texture),
            static_cast<VkImageView>(depth.view),
            static_cast<uint32_t>(depth.width),
            static_cast<uint32_t>(depth.height),
            static_cast<VkFormat>(depth.format),
            L"FSR2_InputDepth",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr2DispatchDescription.motionVectors = ffxGetTextureResourceVK(
            &fsr2Context,
            static_cast<VkImage>(motionVectors.texture),
            static_cast<VkImageView>(motionVectors.view),
            static_cast<uint32_t>(motionVectors.width),
            static_cast<uint32_t>(motionVectors.height),
            static_cast<VkFormat>(motionVectors.format),
            L"FSR2_InputMotionVectors",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        if (false)
        {
            fsr2DispatchDescription.exposure = ffxGetTextureResourceVK(
                &fsr2Context,
                VK_NULL_HANDLE,
                VK_NULL_HANDLE,
                1,
                1,
                VK_FORMAT_UNDEFINED,
                L"FSR2_InputExposure");
        }
        else
        {
            fsr2DispatchDescription.exposure = ffxGetTextureResourceVK(
                &fsr2Context,
                static_cast<VkImage>(exposure.texture),
                static_cast<VkImageView>(exposure.view),
                static_cast<uint32_t>(exposure.width),
                static_cast<uint32_t>(exposure.height),
                static_cast<VkFormat>(exposure.format),
                L"FSR2_InputExposure",
                FFX_RESOURCE_STATE_COMPUTE_READ);
        }

        if (true)
        {
            fsr2DispatchDescription.reactive = ffxGetTextureResourceVK(
                &fsr2Context,
                VK_NULL_HANDLE,
                VK_NULL_HANDLE,
                1,
                1,
                VK_FORMAT_UNDEFINED,
                L"FSR2_EmptyInputReactiveMap");
        }
        else
        {
            // TODO
        }

        if (true)
        {
            fsr2DispatchDescription.transparencyAndComposition = ffxGetTextureResourceVK(
                &fsr2Context,
                VK_NULL_HANDLE,
                VK_NULL_HANDLE,
                1,
                1,
                VK_FORMAT_UNDEFINED,
                L"FSR2_EmptyTransparencyAndCompositionMap");
        }
        else
        {
            // TODO
        }

        const TemporalSuperSamplingConstants& constants = fsr2->constants;
        fsr2DispatchDescription.commandList = ffxGetCommandListVK(static_cast<VkCommandBuffer>(commandList));
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