#pragma once

#include "RealTimeRendererCommon.h"

namespace Horizon
{
    struct TemporalSuperSamplingOptimalSettings
    {
        float optimalRenderResolutionPercentage;
        uint32 optimalRenderWidth;
        uint32 optimalRenderHeight;
        //float optimalSharpness;
    };

    struct TemporalSuperSamplingOptions
    {
        uint32 targetWidth;
        uint32 targetHeight;
    };

    struct TemporalSuperSamplingConstants
    {
        bool reset;
        float jitterOffsetX;
        float jitterOffsetY;
        float motionVectorScaleX;
        float motionVectorScaleY;
        float sharpness;
        float deltaTime;
        float preExposure;
        uint32 renderWidth;
        uint32 renderHeight;
        float cameraNearClippingPlane;
        float cameraFarClippingPlane;
        float cameraFovAngleVertical;
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
        virtual void SetOptions(const TemporalSuperSamplingOptions& options) = 0;
        virtual void SetConstants(const TemporalSuperSamplingConstants& constants) = 0;
        virtual TemporalSuperSamplingOptimalSettings GetOptimalSettings() const = 0;
        virtual uint32 GetJitterPhaseCount(uint32 renderWidth, uint32 targetWidth) const = 0;
        virtual Vector2 GetJitterOffset(uint32 index, uint32 phaseCount) const = 0;
        virtual RenderGraphTextureHandle Dispatch(RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchDescription& dispatchDescription) = 0;
    };

    RenderGraphTextureHandle DispatchCustomTemporalSuperSampling(TemporalSuperSamplingInterface* interface, RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchDescription& dispatchDescription);
}