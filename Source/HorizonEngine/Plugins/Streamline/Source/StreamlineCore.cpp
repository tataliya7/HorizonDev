// #include "StreamlineCore.h"
// #include "StreamlineUtils.h"

// #include <sl_hooks.h>
// #include <sl_version.h>
// #include <sl_reflex.h>
// #include <sl_dlss.h>
// #include <sl_dlss_g.h>
// #include <sl_helpers.h>

// #define STREAMLINE_CEHCK(slFunction) { sl::Result result = slFunction; if (result != sl::Result::eOk) { LogError(GLogger, std::format("Streamline function returns a runtime error. Result: {}. Function: {}. File: {}. Line: {}.", sl::getResultAsStr(result), slFunction, __FILE__, __LINE__)); } }

// namespace Horizon
// {
//     sl::ReflexOptions gActiveReflexOptions;

//     static void UpdateReflexOptionsIfChanged(const sl::ReflexOptions& reflexOptions)
//     {
//         bool isReflexOptionsChanged =
//             (gActiveReflexOptions.mode != reflexOptions.mode) ||
//             (gActiveReflexOptions.useMarkersToOptimize != reflexOptions.useMarkersToOptimize) ||
//             (gActiveReflexOptions.virtualKey != reflexOptions.virtualKey) ||
//             (gActiveReflexOptions.idThread != reflexOptions.idThread);

//         if (isReflexOptionsChanged)
//         {
//             sl::Result result = slReflexSetOptions(reflexOptions);
//             assert(result == sl::Result::eOk, std::format("slReflexSetOptions error: {}", sl::getResultAsStr(result)));
//             gActiveReflexOptions = reflexOptions;
//         }
//     }

//     void StreamlineTick()
//     {
//         if ()
//         {
//             sl::ReflexOptions reflexOptions = {};
//             UpdateReflexOptionsIfChanged(reflexOptions);

//             sl::FrameToken* frameToken = FStreamlineCoreModule::GetStreamlineRHI()->GetFrameToken(GFrameCounter);
//             sl::Result result = slReflexSleep(*frameToken);
//             assert(result == sl::Result::eOk, std::format("slReflexSleep error: {}", sl::getResultAsStr(result)));
//             LastRealTimeAfterSleep = FPlatformTime::Seconds();
//         }
//         else
//         {
//             sl::ReflexOptions reflexOptions = {};
//             reflexOptions.mode = sl::ReflexMode::eOff;
//             UpdateReflexOptionsIfChanged(reflexOptions);
//         }
//     }

//     void SetStreamlineConstants(const StreamlineConstants& constants)
//     {
//         sl::Constants slConstants = {};
//         slConstants.cameraViewToClip = ToSL(constants.cameraViewToClip);
//         slConstants.clipToCameraView = ToSL(constants.clipToCameraView);
//         slConstants.clipToLensClip = ToSL(constants.clipToLensClip);
//         slConstants.clipToPrevClip = ToSL(constants.clipToPrevClip);
//         slConstants.prevClipToClip = ToSL(constants.prevClipToClip);
//         slConstants.jitterOffset = ToSL(constants.jitterOffset);
//         slConstants.mvecScale = ToSL(constants.motionVectorScale);
//         slConstants.cameraPinholeOffset = ToSL(constants.cameraPinholeOffset);
//         slConstants.cameraPos = ToSL(constants.cameraPosition);
//         slConstants.cameraUp = ToSL(constants.cameraUp);
//         slConstants.cameraRight = ToSL(constants.cameraRight);
//         slConstants.cameraFwd = ToSL(constants.cameraForward);
//         slConstants.cameraNear = constants.cameraNear;
//         slConstants.cameraFar = constants.cameraFar;
//         slConstants.cameraFOV = constants.cameraFOV;
//         slConstants.cameraAspectRatio = constants.cameraAspectRatio;
//         slConstants.motionVectorsInvalidValue = FLT_MIN; // TODO: Check if this is correct
//         slConstants.depthInverted = ToSL(constants.depthInverted);
//         slConstants.cameraMotionIncluded = sl::Boolean::eTrue;
//         slConstants.motionVectors3D = sl::Boolean::eFalse;
//         slConstants.reset = ToSL(constants.reset);
//         slConstants.orthographicProjection = sl::Boolean::eFalse;
//         slConstants.motionVectorsDilated = sl::Boolean::eFalse;
//         slConstants.motionVectorsJittered = sl::Boolean::eFalse;

//         sl::FrameToken* frameToken = nullptr;
//         sl::ViewportHandle viewport = 0; // TODO
//         STREAMLINE_CEHCK(slGetNewFrameToken(frameToken, &constants.frameID));
//         STREAMLINE_CEHCK(slSetConstants(slConstants, *frameToken, viewport));
//     }
// }

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

#define NGX_CHECK(result) assert(result == NVSDK_NGX_Result_Success)

static void StreamlineLogMessageCallback(sl::LogType type, const char* msg)
{
    switch (type)
    {
    case sl::LogType::eError:
        HE::LogError(HE::GLogger, std::format("Streamline: {}", msg));
        break;
    case sl::LogType::eWarn:
        HE::LogWarning(HE::GLogger, std::format("Streamline: {}", msg));
        break;
    case sl::LogType::eInfo:
        HE::LogInfo(HE::GLogger, std::format("Streamline: {}", msg));
        break;
        break;
    }
}

static void myAPIErrorCallback(const sl::APIError& e)
{
    // Handle error, use e.hres with DirectX and e.vkRes on Vulkan
    printf("VkResult %d\n", e.vkRes);
};

    bool VulkanRenderBackendCommandListContext::RenderBackendCommandSuperSampling(const RenderBackendCommandSuperSampling& command)
    {
#if HE_ENBALE_STREAMLINE_SUPPORT

        //static NVSDK_NGX_Parameter* NewNGXParameter = nullptr;
        //static NVSDK_NGX_Handle* NewNGXFeatureHandle = nullptr;

        //if (NewNGXFeatureHandle == nullptr)
        //{
        //    bool bReleaseMemoryOnDelete = true;
        //    bool bUseAutoExposure = false;
        //    NVSDK_NGX_PerfQuality_Value PerfQuality = NVSDK_NGX_PerfQuality_Value_MaxQuality;
        //    uint32 DLAAPreset = NVSDK_NGX_DLSS_Hint_Render_Preset_Default;
        //    uint32 DLSSPreset = NVSDK_NGX_DLSS_Hint_Render_Preset_A;
        //    uint32 renderWidth = command.renderWidth;
        //    uint32 renderHeight = command.renderHeight;
        //    uint32 targetWidth = renderWidth;
        //    uint32 targetHeight = renderHeight;
        //    bool depthInverted = true;

        //    NVSDK_NGX_Result ResultAllocateParameters = NVSDK_NGX_VULKAN_AllocateParameters(&NewNGXParameter);
        //    assert(NVSDK_NGX_SUCCEED(ResultAllocateParameters));

        //    NVSDK_NGX_Parameter_SetI(NewNGXParameter, NVSDK_NGX_Parameter_FreeMemOnReleaseFeature, bReleaseMemoryOnDelete ? 1 : 0);
        //    NVSDK_NGX_Parameter_SetUI(NewNGXParameter, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA, static_cast<uint32>(DLAAPreset));
        //    NVSDK_NGX_Parameter_SetUI(NewNGXParameter, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality, static_cast<uint32>(DLSSPreset));
        //    NVSDK_NGX_Parameter_SetUI(NewNGXParameter, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced, static_cast<uint32>(DLSSPreset));
        //    NVSDK_NGX_Parameter_SetUI(NewNGXParameter, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance, static_cast<uint32>(DLSSPreset));
        //    NVSDK_NGX_Parameter_SetUI(NewNGXParameter, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance, static_cast<uint32>(DLSSPreset));

        //    NVSDK_NGX_PerfQuality_Value PerfQualityValue = static_cast<NVSDK_NGX_PerfQuality_Value>(PerfQuality);
        //    assert((PerfQualityValue >= NVSDK_NGX_PerfQuality_Value_MaxPerf) && (PerfQualityValue <= NVSDK_NGX_PerfQuality_Value_UltraQuality));

        //    uint32 FeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_None;
        //    FeatureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_IsHDR;
        //    FeatureCreateFlags |= depthInverted ? NVSDK_NGX_DLSS_Feature_Flags_DepthInverted : 0;
        //    FeatureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_MVLowRes;
        //    FeatureCreateFlags |= bUseAutoExposure ? NVSDK_NGX_DLSS_Feature_Flags_AutoExposure : 0;
        //    //FeatureCreateFlags |= NVSDK_NGX_DLSS_Feature_Flags_DoSharpening;

        //    NVSDK_NGX_DLSS_Create_Params DlssCreateParams = {};
        //    DlssCreateParams.Feature.InWidth = renderWidth;
        //    DlssCreateParams.Feature.InHeight = renderHeight;
        //    DlssCreateParams.Feature.InTargetWidth = targetWidth;
        //    DlssCreateParams.Feature.InTargetHeight = targetHeight;
        //    DlssCreateParams.Feature.InPerfQualityValue = PerfQualityValue;
        //    DlssCreateParams.InFeatureCreateFlags = FeatureCreateFlags;
        //    DlssCreateParams.InEnableOutputSubrects = true;

        //    uint32 CreationNodeMask = 1;
        //    uint32 VisibilityMask = 1;

        //    NVSDK_NGX_Result ResultCreateDLSS = NGX_VULKAN_CREATE_DLSS_EXT(
        //        commandBuffer,
        //        CreationNodeMask,
        //        VisibilityMask,
        //        &NewNGXFeatureHandle,
        //        NewNGXParameter,
        //        &DlssCreateParams);
        //    assert(NVSDK_NGX_SUCCEED(ResultCreateDLSS));
        //}

        // slAllocateResources(commandBuffer, sl::kFeatureDLSS, viewport1->handle);
        // TODO: slFreeResources(sl::kFeatureDLSS, viewport->handle);

        uint32 targetWidth = command.targetWidth;
        uint32 targetHeight = command.targetHeight;

        sl::ViewportHandle viewport = 0;

        sl::Result result;

        static bool first = true;
        if (first)
        {
            device->WaitIdle();
            slFreeResources(sl::kFeatureDLSS_G, viewport);
            first = false;
        }

        sl::DLSSGOptions options{};
        // These are populated based on user selection in the UI
        options.mode = sl::DLSSGMode::eOn;
        options.onErrorCallback = myAPIErrorCallback;

        // IMPORTANT: Note that we are using IDENTICAL viewport as when tagging our resources
        result = slDLSSGSetOptions(viewport, options);
        if (result != sl::Result::eOk)
        {
            printf("slDLSSGSetOptions\n");
            // Handle error here, check the logs
        }

        sl::DLSSGState state = {};
        result = slDLSSGGetState(viewport, state, &options);
        if (result != sl::Result::eOk)
        {
            printf("slDLSSGGetState\n");
        }

        sl::ReflexState reflexState = {};
        result = slReflexGetState(reflexState);
        if (result != sl::Result::eOk)
        {
            printf("slReflexGetState\n");
        }

        VulkanTexture* outputVulkanTexture = device->GetTexture(command.output);
        VulkanTexture* colorVulkanTexture = device->GetTexture(command.color);
        VulkanTexture* depthVulkanTexture = device->GetTexture(command.depth);
        VulkanTexture* motionVectorsVulkanTexture = device->GetTexture(command.motionVectors);

        auto SLTextureFromVulkanTexture = [](VulkanTexture* texture, bool uav)
        {
            sl::Resource slTexture = {};
            slTexture.type = sl::ResourceType::eTex2d;
            slTexture.native = texture->handle;
            slTexture.memory = texture->deivceMemory;
            slTexture.view = uav ? texture->uavs[0].uav : texture->srv;
            slTexture.state = uav ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            slTexture.width = texture->width;
            slTexture.height = texture->height;
            slTexture.nativeFormat = texture->format;
            slTexture.mipLevels = texture->mipLevels;
            slTexture.arrayLayers = texture->arrayLayers;
            //slTexture.gpuVirtualAddress = texture->;
            slTexture.flags = texture->info.flags;
            slTexture.usage = texture->info.usage;
            return slTexture;
        };

        /*sl::Resource output = sl::Resource(sl::ResourceType::eTex2d, outputVulkanTexture->handle, &outputVulkanTexture->deivceMemory, &outputVulkanTexture->uavs[0].uav, VK_IMAGE_LAYOUT_GENERAL);
        sl::Resource color = sl::Resource(sl::ResourceType::eTex2d, colorVulkanTexture->handle, &colorVulkanTexture->deivceMemory, &colorVulkanTexture->srv, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        sl::Resource depth = sl::Resource(sl::ResourceType::eTex2d, depthVulkanTexture->handle, &depthVulkanTexture->deivceMemory, &depthVulkanTexture->srv, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        sl::Resource motionVectors = sl::Resource(sl::ResourceType::eTex2d, motionVectorsVulkanTexture->handle, &motionVectorsVulkanTexture->deivceMemory, &motionVectorsVulkanTexture->srv, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);*/
        sl::Resource output = SLTextureFromVulkanTexture(outputVulkanTexture, true);
        sl::Resource color = SLTextureFromVulkanTexture(colorVulkanTexture, false);
        sl::Resource depth = SLTextureFromVulkanTexture(depthVulkanTexture, false);
        sl::Resource motionVectors = SLTextureFromVulkanTexture(motionVectorsVulkanTexture, false);

        sl::Extent renderExtent = {};
        renderExtent.top = 0;
        renderExtent.left = 0;
        renderExtent.width = colorVulkanTexture->width;
        renderExtent.height = colorVulkanTexture->height;

        sl::Extent targetExtent = {};
        targetExtent.top = 0;
        targetExtent.left = 0;
        targetExtent.width = outputVulkanTexture->width;
        targetExtent.height = outputVulkanTexture->height;

        {
            sl::ResourceTag outputTag = sl::ResourceTag(&output, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
            sl::ResourceTag colorTag = sl::ResourceTag(&color, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent);
            sl::ResourceTag depthTag = sl::ResourceTag(&depth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent);
            sl::ResourceTag motionVectorsTag = sl::ResourceTag(&motionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent);
            sl::ResourceTag tags[] = { outputTag, colorTag, depthTag, motionVectorsTag };
            slSetTag(viewport, tags, _countof(tags), commandBuffer);
        }

        {
            //sl::ResourceTag hudLessColor = sl::ResourceTag(nullptr, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, nullptr);
            sl::ResourceTag hudLessColor = sl::ResourceTag(&output, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
            sl::ResourceTag tags[] = { hudLessColor };
            slSetTag(viewport, tags, _countof(tags), commandBuffer);
        }

        {
            sl::ResourceTag uiColorAndAlpha = sl::ResourceTag(nullptr, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent, nullptr);
            //sl::ResourceTag uiColorAndAlpha = sl::ResourceTag(&output, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
            sl::ResourceTag tags[] = { uiColorAndAlpha };
            slSetTag(viewport, tags, _countof(tags), commandBuffer);
        }

        //{
        //    // Note: here `precisionInfo` refers to the transform needed to be applied to the buffer values to convert from a low-precision format (e.g. 8-bits) to a high-precision format (e.g. 16-bits). Refer to
        //    sl::ResourceTag bidirectionalDistortionTag = sl::ResourceTag(nullptr, sl::kBufferTypeBidirectionalDistortionField, sl::ResourceLifecycle::eValidUntilPresent, &fullExtent, &precisionInfo); // valid all the time
        //    sl::Resource inputs[] = { bidirectionalDistortionTag };
        //    slSetTag(viewport, inputs, _countof(inputs), cmdList);
        //}

        sl::FrameToken* frameToken = nullptr;
        if (SL_FAILED(result, slGetNewFrameToken(frameToken, &command.frameIndex)))
        {
            LogError(GLogger, std::format("slGetNewFrameToken, error code: {}", (int32)result));
            return false;
        }

        // Inform SL that DLSS should be injected at this point for the given viewport
        const sl::BaseStructure* inputs[] = { &viewport };
        if (SL_FAILED(result, slEvaluateFeature(sl::kFeatureDLSS, *frameToken, inputs, _countof(inputs), commandBuffer)))
        {
            LogError(GLogger, std::format("slEvaluateFeature, error code: {}", (int32)result));
            return false;
        }

        //auto NGXTextureFromVulkanTexture = [](VulkanTexture* Texture, bool uav)
        //{
        //    // DLSS_TODO Figure out where to get those from if the textures are arrayed or mipped.
        //    assert(Texture->depth == 1);

        //    VkImageSubresourceRange SubresourceRange = {};
        //    SubresourceRange.aspectMask = Texture->aspectMask;
        //    SubresourceRange.baseArrayLayer = 0;
        //    SubresourceRange.layerCount = 1;
        //    SubresourceRange.baseMipLevel = 0;
        //    SubresourceRange.levelCount = 1;

        //    assert(SubresourceRange.layerCount == 1);
        //    assert(SubresourceRange.levelCount == 1);

        //    NVSDK_NGX_Resource_VK NGXTexture = {};
        //    NGXTexture.Type = NVSDK_NGX_RESOURCE_VK_TYPE_VK_IMAGEVIEW;
        //    NGXTexture.Resource.ImageViewInfo.ImageView = uav ? Texture->uavs[0].uav : Texture->srv;
        //    NGXTexture.Resource.ImageViewInfo.Image = Texture->handle;
        //    NGXTexture.Resource.ImageViewInfo.Format = Texture->format;
        //    NGXTexture.Resource.ImageViewInfo.Width = Texture->width;
        //    NGXTexture.Resource.ImageViewInfo.Height = Texture->height;
        //    NGXTexture.ReadWrite = EnumClassHasFlags(Texture->flags, RenderBackendTextureCreateFlags::UnorderedAccess);
        //    NGXTexture.Resource.ImageViewInfo.SubresourceRange = SubresourceRange;

        //    return NGXTexture;
        //};

        //VulkanTexture* output = device->GetTexture(command.output);
        //VulkanTexture* color = device->GetTexture(command.color);
        //VulkanTexture* depth = device->GetTexture(command.depth);
        //VulkanTexture* motionVectors = device->GetTexture(command.motionVectors);
        //// VulkanTexture* exposureTexture = device->GetTexture(command.exposureTexture);

        //NVSDK_NGX_VK_DLSS_Eval_Params DlssEvalParams = {};

        //NVSDK_NGX_Resource_VK InOutput = NGXTextureFromVulkanTexture(output, true);
        //assert(InOutput.ReadWrite == true);
        //NVSDK_NGX_Resource_VK InColor = NGXTextureFromVulkanTexture(color, false);
        //NVSDK_NGX_Resource_VK InDepth = NGXTextureFromVulkanTexture(depth, false);
        //NVSDK_NGX_Resource_VK InMotionVectors = NGXTextureFromVulkanTexture(motionVectors, false);
        //// NVSDK_NGX_Resource_VK InExposureTexture = NGXTextureFromVulkanTexture(exposureTexture, false);

        //DlssEvalParams.Feature.pInOutput = &InOutput;
        //DlssEvalParams.InOutputSubrectBase.X = 0;
        //DlssEvalParams.InOutputSubrectBase.Y = 0;

        //DlssEvalParams.Feature.pInColor = &InColor;
        //DlssEvalParams.InColorSubrectBase.X = 0;
        //DlssEvalParams.InColorSubrectBase.Y = 0;

        //DlssEvalParams.InRenderSubrectDimensions.Width = command.renderWidth;
        //DlssEvalParams.InRenderSubrectDimensions.Height = command.renderHeight;

        //DlssEvalParams.pInDepth = &InDepth;
        //DlssEvalParams.InDepthSubrectBase.X = 0;
        //DlssEvalParams.InDepthSubrectBase.Y = 0;

        //DlssEvalParams.pInMotionVectors = &InMotionVectors;
        //DlssEvalParams.InMVSubrectBase.X = 0;
        //DlssEvalParams.InMVSubrectBase.Y = 0;

        //// DlssEvalParams.pInExposureTexture = command.useAutoExposure ? nullptr : &InExposureTexture;
        //DlssEvalParams.pInExposureTexture = 0;
        ////DlssEvalParams.InPreExposure = InArguments.PreExposure;
        //DlssEvalParams.Feature.InSharpness = 0.0; // Sharpening is deprecated
        //DlssEvalParams.InJitterOffsetX = command.jitterOffsetX;
        //DlssEvalParams.InJitterOffsetY = command.jitterOffsetY;
        //DlssEvalParams.InMVScaleX = command.motionVectorScaleX;
        //DlssEvalParams.InMVScaleY = command.motionVectorScaleY;
        //DlssEvalParams.InReset = command.reset ? 1 : 0;
        //DlssEvalParams.InFrameTimeDeltaInMsec = command.deltaTime * 1000.0f;

        //NVSDK_NGX_Result ResultEvaluate = NGX_VULKAN_EVALUATE_DLSS_EXT(
        //    commandBuffer,
        //    NewNGXFeatureHandle,
        //    NewNGXParameter,
        //    &DlssEvalParams);

        //if (NVSDK_NGX_FAILED(ResultEvaluate))
        //{
        //    return false;
        //}

#endif
        return true;
    }
