#pragma once

#include "RealTimeRendererCommon.h"

namespace Horizon
{
    constexpr static float SuperResolutionMinPercentage = 0.25f;
    constexpr static float SuperResolutionMaxPercentage = 1.0f;

    struct TemporalSuperSamplingOptimalSettings
    {
        uint32 optimalRenderWidth;
        uint32 optimalRenderHeight;
        float optimalRenderResolutionPercentage;
        //float optimalSharpness;
    };

    struct TemporalSuperSamplingOptions
    {
        uint32 qualityMode;
        float desiredRenderResolutionPercentage;
        uint32 outputWidth;
        uint32 outputHeight;
        float preExposure; // DLSS requires pre-exposure to be set in DLSSOptions.
    };

    struct TemporalSuperSamplingConstants
    {
        bool reset;
        uint32 frameIndex;
        float sharpness;
        float deltaTime;
        float preExposure;
        uint32 renderWidth;
        uint32 renderHeight;
        Vector2f jitterOffset;
        Vector2f motionVectorScale; // Scale factors are used to normalize motion vectors (so that the values are in the [-1,1] range).
        float cameraNearClippingPlane;
        float cameraFarClippingPlane;
        float cameraFovAngleVertical;
        float cameraAspectRatio;
        Vector3f cameraPosition;
        Vector3f cameraUpVector;
        Vector3f cameraRightVector;
        Vector3f cameraForwardVector;
        Matrix4x4f nonJitteredViewToClipMatrix;
        Matrix4x4f nonJitteredClipToViewMatrix;
        Matrix4x4f reprojectionMatrix;
        Matrix4x4f inverseReprojectionMatrix;
    };

    struct TemporalSuperSamplingDispatchDescription
    {
        RenderGraphTextureHandle colorTexture;
        RenderGraphTextureHandle depthTexture;
        RenderGraphTextureHandle motionVectorTexture;
        RenderGraphTextureHandle exposureTexture;
    };

    class TemporalSuperSamplingInterface
    {
    public:
        TemporalSuperSamplingInterface() = default;
        virtual ~TemporalSuperSamplingInterface() = default;
        virtual void SetOptions(const TemporalSuperSamplingOptions& options) = 0;
        virtual void SetConstants(const TemporalSuperSamplingConstants& constants) = 0;
        virtual TemporalSuperSamplingOptimalSettings GetOptimalSettings() const = 0;
        virtual uint32 GetJitterPhaseCount(uint32 renderWidth, uint32 targetWidth) const = 0;
        virtual Vector2f GetJitterOffset(uint32 index, uint32 phaseCount) const = 0;
        virtual RenderGraphTextureHandle Dispatch(RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchDescription& dispatchDescription) = 0;
    };

    uint32 TemporalSuperSamplingGetJitterPhaseCount(uint32 renderWidth, uint32 targetWidth);

    Vector2f TemporalSuperSamplingGetJitterOffset(uint32 index, uint32 phaseCount);

    RenderGraphTextureHandle DispatchCustomTemporalSuperSampling(TemporalSuperSamplingInterface* interface, RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchDescription& dispatchDescription);
}