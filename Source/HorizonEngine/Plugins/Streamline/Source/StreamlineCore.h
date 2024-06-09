#pragma once

#include "Core/CoreModule.h"

namespace Horizon
{
    struct StreamlineVersion
    {
        int32 major = 0;
        int32 minor = 0;
        int32 build = 0;
    };

    enum class StreamlineFeatureSupport : uint8
    {
        Supported,
        NotSupported,
        IncompatibleHardware,
        IncompatiblePlatform,
        DriverOutOfDate,
        OperatingSystemOutOfDate,
        HardewareSchedulingDisabled,
        IncompatibleAPICaptureTool,
        UnknownError,
    };

    enum class FeatureRequirementFlags : uint32
    {
        None = 0,
        D3D12Supported = 1 << 1,
        VulkanSupported = 1 << 2,
        VSyncOffRequired = 1 << 3,
        HardwareSchedulingRequired = 1 << 4
    };

    enum class StreamlineFeature : uint8
    {
        DLSS,
        DLSSG,
        Reflex,
        Count
    };

    enum class StreamlineDLSSQualityMode
    {
        Off = 0,
        Auto = 1,
        Quality = 2,
        Balanced = 3,
        Performance = 4,
        UltraPerformance = 5,
    };

    enum class StreamlineDLSSGMode : uint8
    {
        Off,
        On,
        Auto,
    };

    enum class StreamlineReflexMode : uint8
    {
        Off,
        LowLatency,
        LowLatencyWithBoost,
    };

    enum class StreamlineResourceType
    {
        Depth,
        MotionVectors,
        HUDLessColor,
        UIColorAndAlpha,
        Unknown
    };

    struct StreamlineConstants
    {
        uint32 frameID;

        bool reset;
        bool depthInverted;

        Vector2 jitterOffset;
        Vector2 motionVectorScale;

        Matrix4x4 cameraViewToClip;
        Matrix4x4 clipToCameraView;
        Matrix4x4 clipToLensClip;
        Matrix4x4 clipToPrevClip;
        Matrix4x4 prevClipToClip;

        Vector3 cameraPosition;
        Vector3 cameraUp;
        Vector3 cameraRight;
        Vector3 cameraForward;

        float cameraNear;
        float cameraFar;
        float cameraFOV;
        float cameraAspectRatio;
        Vector2 cameraPinholeOffset;
    };

    class StreamlineContext
    {
    public:
        // DLSS
        StreamlineFeatureSupport StreamlineQueryDLSSSupport();
        bool StreamlineIsDLSSQualityModeSupported(StreamlineDLSSQualityMode mode);
        // DLSSG
        StreamlineFeatureSupport StreamlineQueryDLSSGSupport();
        StreamlineDLSSGMode StreamlineGetDefaultDLSSGMode();
        StreamlineDLSSGMode StreamlineGetDLSSGMode();
        void StreamlineSetDLSSGMode(StreamlineDLSSGMode mode);
        // Reflex
        StreamlineFeatureSupport StreamlineQueryReflexSupport();
        StreamlineReflexMode StreamlineGetDefaultReflexMode();
        StreamlineReflexMode StreamlineGetReflexMode();
        void StreamlineSetReflexMode(StreamlineReflexMode mode);
        float StreamlineGetReflexGameLatencyInMiliseconds();
        float StreamlineGetReflexRenderLatencyInMiliseconds();
        float StreamlineGetReflexGameToRenderLatencyInMiliseconds();
    private:
        void UpdateReflexOptionsIfChanged(const sl::ReflexOptions& reflexOptions);
    };

    struct FidelityFXSuperResolution2Constants
    {
        Matrix4x4 viewToClipMatrix;
        Matrix4x4 clipToViewMatrix;
        Matrix4x4 clipToPreviousClipMatrix;
        Matrix4x4 previousClipToClipMatrix;
        float jitterOffsetX;
        float jitterOffsetY;
        float motionVectorScaleX;
        float motionVectorScaleY;
        Vector3 cameraPosition;
        Vector3 cameraUpVector;
        Vector3 cameraRightVector;
        Vector3 cameraForwardVector;
        float cameraFarClippingPlane;
        float cameraNearClippingPlane;
        float cameraFovAngleVertical;
        float cameraAspectRatio;
        bool reset;
        uint32 renderWidth;
        uint32 renderHeight;
        float deltaTime;
        float preExposure;
    };
}
