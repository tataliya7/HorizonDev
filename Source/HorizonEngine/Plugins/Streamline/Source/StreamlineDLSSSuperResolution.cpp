#include "StreamlineDLSSSuperResolution.h"
#include "StreamlineDLSSSuperResolutionPrivate.h"
#include "StreamlineUtility.h"

#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>

namespace Horizon
{
    StreamlineDLSSSuperResolution::StreamlineDLSSSuperResolution(RenderBackend* renderBackend)
        : renderBackend(renderBackend)
    {

    }

    StreamlineDLSSSuperResolution::~StreamlineDLSSSuperResolution()
    {

    }

    static sl::DLSSMode GetDLSSMode(uint32 mode)
    {
        switch (mode)
        {
        case 0:
            return sl::DLSSMode::eOff;
        case 1:
            return sl::DLSSMode::eMaxPerformance;
        case 2:
            return sl::DLSSMode::eBalanced;
        case 3:
            return sl::DLSSMode::eMaxQuality;
        case 4:
            return sl::DLSSMode::eUltraPerformance;
        case 5:
            return sl::DLSSMode::eUltraQuality;
        case 6:
            return sl::DLSSMode::eDLAA;
        default:
            return sl::DLSSMode::eOff;
        }
    }

    void StreamlineDLSSSuperResolution::SetOptions(const TemporalSuperSamplingOptions& options)
    {
        const sl::ViewportHandle& viewport = 0;

        sl::DLSSMode dlssMode = GetDLSSMode(options.qualityMode);

        sl::DLSSOptions dlssOptions = {};
        dlssOptions.mode = dlssMode;
        dlssOptions.outputWidth = options.outputWidth;
        dlssOptions.outputHeight = options.outputHeight;
        dlssOptions.sharpness = 0.0f;
        dlssOptions.preExposure = options.preExposure;
        dlssOptions.exposureScale = 1.0f; // TODO
        dlssOptions.colorBuffersHDR = sl::Boolean::eTrue;
        dlssOptions.indicatorInvertAxisX = sl::Boolean::eFalse;
        dlssOptions.indicatorInvertAxisY = sl::Boolean::eFalse;
        dlssOptions.dlaaPreset = sl::DLSSPreset::ePresetA;
        dlssOptions.qualityPreset = sl::DLSSPreset::ePresetB;
        dlssOptions.balancedPreset = sl::DLSSPreset::ePresetC;
        dlssOptions.performancePreset = sl::DLSSPreset::ePresetD;
        dlssOptions.ultraPerformancePreset = sl::DLSSPreset::ePresetE;
        dlssOptions.useAutoExposure = sl::Boolean::eFalse; // We don't use DLSS's own auto-exposure.
        if (SL_FAILED(result, slDLSSSetOptions(viewport, dlssOptions)))
        {
            LogError(GLogger, std::format("slDLSSSetOptions, error code: {}.", int32(result)));
        }

        this->options = dlssOptions;
    }

    void StreamlineDLSSSuperResolution::SetConstants(const TemporalSuperSamplingConstants& constants)
    {
        const sl::ViewportHandle& viewport = 0;

        sl::Constants slCommonConstants = {};
        slCommonConstants.cameraViewToClip = ToSL(constants.nonJitteredViewToClipMatrix);
        slCommonConstants.clipToCameraView = ToSL(constants.nonJitteredClipToViewMatrix);
        //slCommonConstants.clipToLensClip = {};
        slCommonConstants.clipToPrevClip = ToSL(constants.reprojectionMatrix);
        slCommonConstants.prevClipToClip = ToSL(constants.inverseReprojectionMatrix);
        slCommonConstants.jitterOffset = sl::float2(constants.jitterOffset.x, constants.jitterOffset.y);
        slCommonConstants.mvecScale = sl::float2(constants.motionVectorScale.x, constants.motionVectorScale.y);
        slCommonConstants.cameraPinholeOffset = sl::float2(0.0f, 0.0f);
        slCommonConstants.cameraPos = sl::float3(constants.cameraPosition.x, constants.cameraPosition.y, constants.cameraPosition.z);
        slCommonConstants.cameraUp = sl::float3(constants.cameraUpVector.x, constants.cameraUpVector.y, constants.cameraUpVector.z);
        slCommonConstants.cameraRight = sl::float3(constants.cameraRightVector.x, constants.cameraRightVector.y, constants.cameraRightVector.z);
        slCommonConstants.cameraFwd = sl::float3(constants.cameraForwardVector.x, constants.cameraForwardVector.y, constants.cameraForwardVector.z);
        slCommonConstants.cameraNear = constants.cameraNearClippingPlane;
        slCommonConstants.cameraFar = constants.cameraFarClippingPlane;
        slCommonConstants.cameraFOV = constants.cameraFovAngleVertical;
        slCommonConstants.cameraAspectRatio = constants.cameraAspectRatio;
        slCommonConstants.motionVectorsInvalidValue = sl::INVALID_FLOAT;
        slCommonConstants.depthInverted = sl::Boolean::eTrue;
        slCommonConstants.cameraMotionIncluded = sl::Boolean::eTrue;
        slCommonConstants.motionVectors3D = sl::Boolean::eFalse;
        slCommonConstants.reset = constants.reset ? sl::Boolean::eTrue : sl::Boolean::eFalse;
        slCommonConstants.orthographicProjection = sl::Boolean::eFalse;
        slCommonConstants.motionVectorsDilated = sl::Boolean::eFalse;
        slCommonConstants.motionVectorsJittered = sl::Boolean::eFalse;

        sl::FrameToken* frameToken = nullptr;
        if (SL_FAILED(result, slGetNewFrameToken(frameToken, &constants.frameIndex)))
        {
            LogError(GLogger, std::format("slGetNewFrameToken, error code: {}.", int32(result)));
        }

        if (SL_FAILED(result, slSetConstants(slCommonConstants, *frameToken, viewport)))
        {
            LogError(GLogger, std::format("slSetConstants, error code: {}.", int32(result)));
        }

        this->constants = constants;
    }

    TemporalSuperSamplingOptimalSettings StreamlineDLSSSuperResolution::GetOptimalSettings() const
    {
        uint32 targetWidth = options.outputWidth;
        uint32 targetHeight = options.outputHeight;

        sl::DLSSOptimalSettings dlssOptimalSettings = {};
        if (SL_FAILED(result, slDLSSGetOptimalSettings(options, dlssOptimalSettings)))
        {
            LogError(GLogger, std::format("slDLSSGetOptimalSettings, error code: {}.", int32(result)));
        }

        uint32 renderWidth = dlssOptimalSettings.optimalRenderWidth;
        uint32 renderHeight = dlssOptimalSettings.optimalRenderHeight;

        float upscaleRatio = float(targetWidth) / float(renderWidth);
        float renderResolutionPercentage = 1.0f / upscaleRatio;

        TemporalSuperSamplingOptimalSettings optimalSettings =
        {
            .optimalRenderWidth = renderWidth,
            .optimalRenderHeight = renderHeight,
            .optimalRenderResolutionPercentage = renderResolutionPercentage
        };

        return optimalSettings;
    }

    uint32 StreamlineDLSSSuperResolution::GetJitterPhaseCount(uint32 renderWidth, uint32 targetWidth) const
    {
        return TemporalSuperSamplingGetJitterPhaseCount(renderWidth, targetWidth);
    }

    Vector2f StreamlineDLSSSuperResolution::GetJitterOffset(uint32 index, uint32 phaseCount) const
    {
        return TemporalSuperSamplingGetJitterOffset(index, phaseCount);
    }

    RenderGraphTextureHandle StreamlineDLSSSuperResolution::Dispatch(
        RenderGraph& renderGraph,
        const SceneView& view,
        const TemporalSuperSamplingDispatchDescription& dispatchDescription)
    {
        RenderBackendDispatchSuperSamplingCallback dispatchCallback = nullptr;
        switch (renderBackend->GetType())
        {
        case RenderBackendType::D3D12: dispatchCallback = StreamlineDLSSSuperResolutionDispatchD3D12; break;
        case RenderBackendType::Vulkan: dispatchCallback = StreamlineDLSSSuperResolutionDispatchVulkan; break;
        default: std::unreachable(); break;
        }

        void* dlssContext = this;

        RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
                view.targetWidth,
                view.targetHeight,
                RenderBackendTextureFormat::R16G16B16A16Float,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "DLSSOutputTexture");

        renderGraph.AddPass(
            std::format("Evaluate DLSS ({}x{} -> {}x{})", renderGraph.GetTextureDesc(dispatchDescription.colorTexture).width, renderGraph.GetTextureDesc(dispatchDescription.colorTexture).height, outputTextureDesc.width, outputTextureDesc.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle colorTexture = builder.ReadTexture(dispatchDescription.colorTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle depthTexture = builder.ReadTexture(dispatchDescription.depthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle motionVectorTexture = builder.ReadTexture(dispatchDescription.motionVectorTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle exposureTexture = builder.ReadTexture(dispatchDescription.exposureTexture, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.DispatchSuperSampling(
                        dlssContext,
                        dispatchCallback,
                        registry.GetRenderBackendTextureHandle(outputTexture),
                        registry.GetRenderBackendTextureHandle(colorTexture),
                        registry.GetRenderBackendTextureHandle(depthTexture),
                        registry.GetRenderBackendTextureHandle(motionVectorTexture),
                        registry.GetRenderBackendTextureHandle(exposureTexture));
                };
            });

        return outputTexture;
    }

    TemporalSuperSamplingInterface* StreamlineDLSSSuperResolutionCreate(RenderBackend* renderBackend)
    {
        return new StreamlineDLSSSuperResolution(renderBackend);
    }

    void StreamlineDLSSSuperResolutionDestroy(TemporalSuperSamplingInterface* temporalSuperSamplingInterface)
    {
        delete reinterpret_cast<StreamlineDLSSSuperResolution*>(temporalSuperSamplingInterface);
    }
}