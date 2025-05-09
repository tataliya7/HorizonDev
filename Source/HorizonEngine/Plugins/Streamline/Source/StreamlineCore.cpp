// #include "StreamlineCore.h"
// #include "StreamlineUtils.h"

// #include <sl_hooks.h>
// #include <sl_version.h>
// #include <sl_reflex.h>
// #include <sl_dlss.h>
// #include <sl_dlss_g.h>
// #include <sl_helpers.h>

// #define STREAMLINE_CEHCK(slFunction) { sl::Result result = slFunction; if (result != sl::Result::eOk) { LogError(GLogger, std::format("Streamline function returns a runtime error. Result: {}. Function: {}. File: {}. Line: {}.", sl::getResultAsStr(result), slFunction, __FILE__, __LINE__)); } }

#if 0
{
    // Init Streamline
    {
        sl::Feature features[] = { sl::kFeatureDLSS, sl::kFeatureDLSS_G, sl::kFeatureReflex };

        sl::Preferences pref = {};
        pref.showConsole = true;
        pref.logLevel = sl::LogLevel::eDefault;
        pref.pathsToPlugins = nullptr;
        pref.numPathsToPlugins = 0;
        pref.pathToLogsAndData = nullptr;
        pref.allocateCallback = nullptr;
        pref.releaseCallback = nullptr;
        pref.logMessageCallback = StreamlineLogMessageCallback;
        pref.flags = sl::PreferenceFlags::eDisableCLStateTracking;// | sl::PreferenceFlags::eAllowOTA;
        pref.featuresToLoad = features;
        pref.numFeaturesToLoad = _countof(features);
        pref.applicationId = sl::INVALID_UINT;
        pref.engine = sl::EngineType::eCustom;
        pref.engineVersion = "Horizon Engine";
        pref.projectId = "a0f57b54-1daf-4934-90ae-c4035c19df04";
        pref.renderAPI = sl::RenderAPI::eVulkan;

        sl::Result result;
        if (SL_FAILED(result, slInit(pref, sl::kSDKVersion)))
        {
            LogInfo(GLogger, std::format("slInit, error code: {}.", (int32)result));
        }

        //{
        //    sl::FeatureRequirements requirements = {};
        //    if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureNRD, requirements)))
        //    {
        //        LogInfo(GLogger, std::format("slGetFeatureRequirements, error code: {}.", (int32)result));
        //    }
        //    else
        //    {
        //        // Feature is loaded, we can check the requirements
        //        assert(requirements.flags & sl::FeatureRequirementFlags::eVulkanSupported);

        //        for (uint32 i = 0; i < requirements.vkNumInstanceExtensions; i++)
        //        {
        //            printf("%s\n", requirements.vkInstanceExtensions[i]);
        //        }
        //        for (uint32 i = 0; i < requirements.vkNumDeviceExtensions; i++)
        //        {
        //            printf("%s\n", requirements.vkDeviceExtensions[i]);
        //        }
        //    }
        //}

        {
            sl::FeatureRequirements requirements = {};
            if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureDLSS, requirements)))
            {
                LogInfo(GLogger, std::format("slGetFeatureRequirements, error code: {}.", (int32)result));
            }
            else
            {
                // Feature is loaded, we can check the requirements
                assert(requirements.flags & sl::FeatureRequirementFlags::eVulkanSupported);

                for (uint32 i = 0; i < requirements.vkNumInstanceExtensions; i++)
                {
                    printf("%s\n", requirements.vkInstanceExtensions[i]);
                }
                for (uint32 i = 0; i < requirements.vkNumDeviceExtensions; i++)
                {
                    printf("%s\n", requirements.vkDeviceExtensions[i]);
                }
            }
        }

        {
            sl::FeatureRequirements requirements = {};
            if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureDLSS_G, requirements)))
            {
                LogInfo(GLogger, std::format("slGetFeatureRequirements, error code: {}.", (int32)result));
            }
            else
            {
                // Feature is loaded, we can check the requirements
                assert(requirements.flags & sl::FeatureRequirementFlags::eVulkanSupported);

                for (uint32 i = 0; i < requirements.vkNumInstanceExtensions; i++)
                {
                    printf("%s\n", requirements.vkInstanceExtensions[i]);
                }
                for (uint32 i = 0; i < requirements.vkNumDeviceExtensions; i++)
                {
                    printf("%s\n", requirements.vkDeviceExtensions[i]);
                }
            }
        }

        {
            sl::FeatureRequirements requirements = {};
            if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureReflex, requirements)))
            {
                LogInfo(GLogger, std::format("slGetFeatureRequirements, error code: {}.", (int32)result));
            }
            else
            {
                // Feature is loaded, we can check the requirements
                assert(requirements.flags & sl::FeatureRequirementFlags::eVulkanSupported);

                for (uint32 i = 0; i < requirements.vkNumInstanceExtensions; i++)
                {
                    printf("%s\n", requirements.vkInstanceExtensions[i]);
                }
                for (uint32 i = 0; i < requirements.vkNumDeviceExtensions; i++)
                {
                    printf("%s\n", requirements.vkDeviceExtensions[i]);
                }
            }
        }

        {
            sl::ReflexState state = {};
            if (SL_FAILED(result, slReflexGetState(state)))
            {
                LogInfo(GLogger, std::format("slReflexGetState, error code: {}", (int32)result));
            }
            if (state.lowLatencyAvailable)
            {
                //
                // Reflex Low Latency is available, on NVDA hardware this would be done through Reflex.
                //
                // The application can show the Reflex Low Latency UI. (Otherwise hide/disable the UI.)
                // This is for UI only. Do everything else the same, even when this is false.
                //
            }
            if (state.flashIndicatorDriverControlled)
            {
                //
                // Reflex Flash Indicator (RFI) is controlled by the driver. This means
                // the application should always check for left mouse button clicks and
                // send the trigger flash markers accordingly. The driver will decide
                // whether to show the RFI on screen based on user preference.
                //
            }
        }
        //// We are using NULL adapter on purpose
        //sl::AdapterInfo adapterInfo = {};
        //if (SL_FAILED(result, slIsFeatureSupported(sl::Feature::eDLSS, adapterInfo)))
        //{
        //    // Requested feature is not supported, let's see why
        //    switch (result)
        //    {
        //    case sl::Result::eErrorOSOutOfDate:              // inform user to update OS
        //    case sl::Result::eErrorDriverOutOfDate:          // inform user to update driver
        //    case sl::Result::eErrorNoSupportedAdapterFound:  // cannot use any available adapter
        //        // and so on ...
        //    };
        //}
        //else
        //{
        //    // Feature is supported on at least one adapter so now we need to figure out which one before we create our device.
        //}
    }
}
#endif

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

