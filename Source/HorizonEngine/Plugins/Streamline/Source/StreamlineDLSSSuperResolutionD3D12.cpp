#include "StreamlineDLSSSuperResolution.h"
#include "StreamlineDLSSSuperResolutionPrivate.h"

#include <sl.h>
#include <sl_consts.h>
#include <sl_hooks.h>
#include <sl_version.h>
#include <sl_dlss.h>
#include <sl_helpers.h>

//static void StreamlineErrorCallbackVulkanD3D12(const sl::APIError& e)
//{
//    // Handle error, use e.hres with DirectX and e.vkRes on Vulkan
//    printf("HRESULT %d\n", e.hres);
//};

namespace Horizon
{
    bool StreamlineDLSSSuperResolutionDispatchD3D12(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors,
        const RenderBackendTextureResource& exposure)
    {
        StreamlineDLSSSuperResolution* dlssContext = static_cast<StreamlineDLSSSuperResolution*>(context);

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

        sl::Resource resourceOutputColor = sl::Resource(sl::ResourceType::eTex2d, output.texture, output.state);
        sl::Resource resourceInputColor = sl::Resource(sl::ResourceType::eTex2d, color.texture, color.state);
        sl::Resource resourceDepth = sl::Resource(sl::ResourceType::eTex2d, depth.texture, depth.state);
        sl::Resource resourceMotionVectors = sl::Resource(sl::ResourceType::eTex2d, motionVectors.texture, motionVectors.state);
        sl::Resource resourceExposure = sl::Resource(sl::ResourceType::eTex2d, exposure.texture, exposure.state);

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
        //     slSetTag(viewport, tags, _countof(tags), commandBuffer);
        // }
        //
        // {
        //     sl::ResourceTag uiColorAndAlpha = sl::ResourceTag(nullptr, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent, nullptr);
        //     //sl::ResourceTag uiColorAndAlpha = sl::ResourceTag(&output, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
        //     sl::ResourceTag tags[] = { uiColorAndAlpha };
        //     slSetTag(viewport, tags, _countof(tags), commandBuffer);
        // }

        //{
        //    // Note: here `precisionInfo` refers to the transform needed to be applied to the buffer values to convert from a low-precision format (e.g. 8-bits) to a high-precision format (e.g. 16-bits). Refer to
        //    sl::ResourceTag bidirectionalDistortionTag = sl::ResourceTag(nullptr, sl::kBufferTypeBidirectionalDistortionField, sl::ResourceLifecycle::eValidUntilPresent, &fullExtent, &precisionInfo); // valid all the time
        //    sl::Resource inputs[] = { bidirectionalDistortionTag };
        //    slSetTag(viewport, inputs, _countof(inputs), cmdList);
        //}

        sl::FrameToken* frameToken = nullptr;
        if (SL_FAILED(result, slGetNewFrameToken(frameToken, &(dlssContext->constants.frameIndex))))
        {
            LogError(GLogger, std::format("slGetNewFrameToken, error code: {}.", int32(result)));
        }

        // Inform SL that DLSS should be injected at this point for the given viewport
        const sl::BaseStructure* inputs[] = { &viewport };
        if (SL_FAILED(result, slEvaluateFeature(sl::kFeatureDLSS, *frameToken, inputs, _countof(inputs), commandList)))
        {
            LogError(GLogger, std::format("slEvaluateFeature, error code: {}.", int32(result)));
            return false;
        }

        return true;
    }
}