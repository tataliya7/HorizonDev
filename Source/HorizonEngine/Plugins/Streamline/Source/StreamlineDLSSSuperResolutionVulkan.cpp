#include "StreamlineDLSSSuperResolution.h"
#include "StreamlineDLSSSuperResolutionPrivate.h"

#include <vulkan/vulkan.h>

#include <sl.h>
#include <sl_consts.h>
#include <sl_hooks.h>
#include <sl_version.h>

// #include <sl_nrd.h>
#include <sl_dlss.h>
#include <sl_dlss_g.h>
#include <sl_reflex.h>
#include <sl_helpers.h>
#include <sl_helpers_vk.h>

namespace Horizon
{
    //static void StreamlineErrorCallbackVulkan(const sl::APIError& e)
    //{
    //    // Handle error, use e.hres with DirectX and e.vkRes on Vulkan
    //    printf("VkResult %d\n", e.vkRes);
    //};

    bool StreamlineDLSSSuperResolutionDispatchVulkan(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure)
    {
        StreamlineDLSSSuperResolution* dlssContext = static_cast<StreamlineDLSSSuperResolution*>(context);

        // slAllocateResources(commandBuffer, sl::kFeatureDLSS, viewport1->handle);
        // TODO: slFreeResources(sl::kFeatureDLSS, viewport->handle);

        sl::ViewportHandle viewport = 0;

        sl::Extent renderExtent = {};
        renderExtent.top = 0;
        renderExtent.left = 0;
        renderExtent.width = color.width;
        renderExtent.height = color.height;

        sl::Extent targetExtent = {};
        targetExtent.top = 0;
        targetExtent.left = 0;
        targetExtent.width = output.width;
        targetExtent.height = output.height;

        sl::Extent exposureTextureExtent = {};
        targetExtent.top = 0;
        targetExtent.left = 0;
        targetExtent.width = 1;
        targetExtent.height = 1;

        auto slGetTextureResourceVulkan = [](const RenderBackendTextureResource& texture)
        {
            sl::Resource slTexture = {};
            slTexture.type = sl::ResourceType::eTex2d;
            slTexture.native = texture.texture;
            slTexture.memory = texture.memory;
            slTexture.view = texture.view;
            slTexture.state = texture.state;
            slTexture.width = texture.width;
            slTexture.height = texture.height;
            slTexture.nativeFormat = texture.format;
            slTexture.mipLevels = texture.mipLevels;
            slTexture.arrayLayers = texture.arrayLayers;
            //slTexture.gpuVirtualAddress = texture->;
            slTexture.flags = texture.flags;
            slTexture.usage = texture.usage;
            return slTexture;
        };

        sl::Resource resourceOutputColor = slGetTextureResourceVulkan(output);
        sl::Resource resourceInputColor = slGetTextureResourceVulkan(color);
        sl::Resource resourceDepth = slGetTextureResourceVulkan(depth);
        sl::Resource resourceMotionVectors = slGetTextureResourceVulkan(motionVectors);
        sl::Resource resourceExposure = slGetTextureResourceVulkan(exposure);

        sl::ResourceTag resourceOutputColorTag = sl::ResourceTag(&resourceOutputColor, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
        sl::ResourceTag resourceInputColorTag = sl::ResourceTag(&resourceInputColor, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent);
        sl::ResourceTag resourceDepthTag = sl::ResourceTag(&resourceDepth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent);
        sl::ResourceTag resourceMotionVectorsTag = sl::ResourceTag(&resourceMotionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent);
        sl::ResourceTag resourceExposureTag = sl::ResourceTag(&resourceExposure, sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilPresent, &exposureTextureExtent);

        sl::ResourceTag dlssResourceTags[] = { resourceOutputColorTag, resourceInputColorTag, resourceDepthTag, resourceMotionVectorsTag, resourceExposureTag };
        slSetTag(viewport, dlssResourceTags, _countof(dlssResourceTags), commandList);

        // {
        //     //sl::ResourceTag hudLessColor = sl::ResourceTag(nullptr, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, nullptr);
        //     sl::ResourceTag hudLessColor = sl::ResourceTag(&output, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
        //     sl::ResourceTag tags[] = { hudLessColor };
        //     slSetTag(viewport, tags, _countof(tags), commandList);
        // }
        //
        // {
        //     sl::ResourceTag uiColorAndAlpha = sl::ResourceTag(nullptr, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent, nullptr);
        //     //sl::ResourceTag uiColorAndAlpha = sl::ResourceTag(&output, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
        //     sl::ResourceTag tags[] = { uiColorAndAlpha };
        //     slSetTag(viewport, tags, _countof(tags), commandList);
        // }

        //{
        //    // Note: here `precisionInfo` refers to the transform needed to be applied to the buffer values to convert from a low-precision format (e.g. 8-bits) to a high-precision format (e.g. 16-bits). Refer to
        //    sl::ResourceTag bidirectionalDistortionTag = sl::ResourceTag(nullptr, sl::kBufferTypeBidirectionalDistortionField, sl::ResourceLifecycle::eValidUntilPresent, &fullExtent, &precisionInfo); // valid all the time
        //    sl::Resource inputs[] = { bidirectionalDistortionTag };
        //    slSetTag(viewport, inputs, _countof(inputs), cmdList);
        //}

        // Inform SL that DLSS should be injected at this point for the given viewport
        const sl::BaseStructure* inputs[] = { &viewport };
        if (SL_FAILED(result, slEvaluateFeature(sl::kFeatureDLSS, *(dlssContext->frameToken), inputs, _countof(inputs), commandList)))
        {
            LogError(GLogger, std::format("slEvaluateFeature, error code: {}.", int32(result)));
            return false;
        }

        return true;
    }
}