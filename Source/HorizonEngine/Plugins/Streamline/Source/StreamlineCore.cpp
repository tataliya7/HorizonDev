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

