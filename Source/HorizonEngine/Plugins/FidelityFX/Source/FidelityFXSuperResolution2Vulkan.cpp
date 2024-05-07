#include "FidelityFXSuperResolution2.h"

#include <ffx_fsr2.h>
#include <vk/ffx_fsr2_vk.h>

namespace FidelityFX
{
    static void FSR2MessageCallBack(FfxFsr2MsgType type, const wchar_t* message)
    {
        if (type == FFX_FSR2_MESSAGE_TYPE_ERROR)
        {
            Horizon::LogError(Horizon::GLogger, std::format(L"FSR2_API_DEBUG_ERROR: {}", message));
        }
        else if (type == FFX_FSR2_MESSAGE_TYPE_WARNING)
        {
            Horizon::LogWarning(Horizon::GLogger, std::format(L"FSR2_API_DEBUG_WARNING: {}", message));
        }
    }

    bool FSR2Execute(const Horizon::RenderBackendSuperSamplingDescription& description)
    {
        static FfxFsr2ContextDescription fsr2InitializationParameters = {};
        static FfxFsr2Context fsr2Context = {};
        static uint32_t previousRenderWidth  = 0;
        static uint32_t previousRenderHeight = 0;
        static uint32_t previousTargetWidth  = 0;
        static uint32_t previousTargetHeight = 0;

        // TODO: Handle multiple viewports
        if (previousRenderWidth != description.renderWidth ||
            previousRenderHeight != description.renderHeight ||
            previousTargetWidth != description.targetWidth ||
            previousTargetHeight != description.targetHeight)
        {
            // Only destroy contexts which are live
            if (fsr2InitializationParameters.callbacks.scratchBuffer != nullptr)
            {
                ffxFsr2ContextDestroy(&fsr2Context);
                free(fsr2InitializationParameters.callbacks.scratchBuffer);
                fsr2InitializationParameters.callbacks.scratchBuffer = nullptr;
            }

            const size_t scratchBufferSize = ffxFsr2GetScratchMemorySizeVK((VkPhysicalDevice)description.physicalDevice);
            void* scratchBuffer = malloc(scratchBufferSize);
            FfxErrorCode errorCode = ffxFsr2GetInterfaceVK(&fsr2InitializationParameters.callbacks, scratchBuffer, scratchBufferSize, (VkPhysicalDevice)description.physicalDevice, vkGetDeviceProcAddr);
            FFX_ASSERT(errorCode == FFX_OK);

            fsr2InitializationParameters.device = ffxGetDeviceVK((VkDevice)description.device);
            fsr2InitializationParameters.maxRenderSize.width = description.renderWidth;
            fsr2InitializationParameters.maxRenderSize.height = description.renderHeight;
            fsr2InitializationParameters.displaySize.width = description.targetWidth;
            fsr2InitializationParameters.displaySize.height = description.targetHeight;
            //fsr2InitializationParameters.flags = FFX_FSR2_ENABLE_AUTO_EXPOSURE;

            // if (m_bInvertedDepth)
            {
                fsr2InitializationParameters.flags |= FFX_FSR2_ENABLE_DEPTH_INVERTED;
            }

#if !HORIZON_CONFIGURATION_RELEASE
            // if (device->fsr2EnableDebugCheck)
            {
                fsr2InitializationParameters.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
                fsr2InitializationParameters.fpMessage = &FSR2MessageCallBack;
            }
#endif

            // Input data is HDR
            fsr2InitializationParameters.flags |= FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;

            //const uint64_t memoryUsageBefore = getMemoryUsageSnapshot(device->GetPhysicalDeviceHandle());
            errorCode = ffxFsr2ContextCreate(&fsr2Context, &fsr2InitializationParameters);
            FFX_ASSERT(errorCode == FFX_OK);
            //const uint64_t memoryUsageAfter = getMemoryUsageSnapshot(device->GetPhysicalDeviceHandle());
            //memoryUsageInMegabytes = (memoryUsageAfter - memoryUsageBefore) * 0.000001f;

            previousRenderWidth = description.renderWidth;
            previousRenderHeight = description.renderHeight;
            previousTargetWidth = description.targetWidth;
            previousTargetHeight = description.targetHeight;
        }

        FfxFsr2DispatchDescription fsr2DispatchDescription = {};

        fsr2DispatchDescription.color = ffxGetTextureResourceVK(
            &fsr2Context,
            (VkImage)description.color.texture,
            (VkImageView)description.color.view,
            description.color.width,
            description.color.height,
            (VkFormat)description.color.format,
            L"FSR2_InputColor",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr2DispatchDescription.depth = ffxGetTextureResourceVK(
            &fsr2Context,
            (VkImage)description.depth.texture,
            (VkImageView)description.depth.view,
            description.depth.width,
            description.depth.height,
            (VkFormat)description.depth.format,
            L"FSR2_InputDepth",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr2DispatchDescription.motionVectors = ffxGetTextureResourceVK(
            &fsr2Context,
            (VkImage)description.motionVectors.texture,
            (VkImageView)description.motionVectors.view,
            description.motionVectors.width,
            description.motionVectors.height,
            (VkFormat)description.motionVectors.format,
            L"FSR2_InputMotionVectors",
            FFX_RESOURCE_STATE_COMPUTE_READ);

        fsr2DispatchDescription.output = ffxGetTextureResourceVK(
            &fsr2Context,
            (VkImage)description.output.texture,
            (VkImageView)description.output.view,
            description.output.width,
            description.output.height,
            (VkFormat)description.output.format,
            L"FSR2_OutputUpscaledColor",
            FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        if (true)
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
            // TODO
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

        fsr2DispatchDescription.commandList = ffxGetCommandListVK((VkCommandBuffer)description.commandList);
        fsr2DispatchDescription.jitterOffset.x = description.jitterOffsetX;
        fsr2DispatchDescription.jitterOffset.y = description.jitterOffsetY;
        fsr2DispatchDescription.motionVectorScale.x = (float)description.renderWidth;
        fsr2DispatchDescription.motionVectorScale.y = (float)description.renderHeight;
        fsr2DispatchDescription.reset = description.reset;
        fsr2DispatchDescription.enableSharpening = description.enableSharpening;
        fsr2DispatchDescription.sharpness = description.sharpeness;
        fsr2DispatchDescription.frameTimeDelta = description.deltaTime * 1000.0f; // 'frameTimeDelta' is expressed in milliseconds
        fsr2DispatchDescription.preExposure = description.preExposure;
        fsr2DispatchDescription.renderSize.width = description.renderWidth;
        fsr2DispatchDescription.renderSize.height = description.renderHeight;
        fsr2DispatchDescription.cameraFar = description.cameraFarClippingPlane;
        fsr2DispatchDescription.cameraNear = description.cameraNearClippingPlane;
        fsr2DispatchDescription.cameraFovAngleVertical = description.cameraFovAngleVertical;
        fsr2DispatchDescription.viewSpaceToMetersFactor = 1.0f;

        if (fsr2InitializationParameters.flags & FFX_FSR2_ENABLE_DEPTH_INVERTED)
        {
            std::swap(fsr2DispatchDescription.cameraFar, fsr2DispatchDescription.cameraNear);
        }

        FfxErrorCode errorCode = ffxFsr2ContextDispatch(&fsr2Context, &fsr2DispatchDescription);
        FFX_ASSERT(errorCode == FFX_OK);

        return (errorCode == FFX_OK);
    }
}