#include "StreamlineDLSS.h"

#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>

namespace Horizon
{
    if (IsDLAAEnabled())
        {
            sl::Result slResult = sl::Result::eOk;
            sl::ViewportHandle slViewport = 0;

            sl::DLSSOptions dlssOptions = {};
            dlssOptions.mode = sl::DLSSMode::eMaxQuality;
            dlssOptions.outputWidth = targetResolution.width;
            dlssOptions.outputHeight = targetResolution.height;
            dlssOptions.sharpness = 0.0f;
            dlssOptions.preExposure = 1.0f;
            dlssOptions.exposureScale = 1.0f;
            dlssOptions.colorBuffersHDR = sl::Boolean::eTrue;
            dlssOptions.indicatorInvertAxisX = sl::Boolean::eFalse;
            dlssOptions.indicatorInvertAxisY = sl::Boolean::eFalse;
            dlssOptions.dlaaPreset = sl::DLSSPreset::ePresetA;
            dlssOptions.qualityPreset = sl::DLSSPreset::ePresetB;
            dlssOptions.balancedPreset = sl::DLSSPreset::ePresetC;
            dlssOptions.performancePreset = sl::DLSSPreset::ePresetD;
            dlssOptions.ultraPerformancePreset = sl::DLSSPreset::ePresetE;
            if (SL_FAILED(result, slDLSSSetOptions(slViewport, dlssOptions)))
            {
                LogError(GLogger, std::format("slDLSSSetOptions, error code: {}", (int32)result));
            }
        }

        if (IsDLSSEnabled() || IsDLAAEnabled())
        {
            Matrix4x4 reprojectionMatrix = perFrameShaderParameters.nonJitteredInvViewProjectionMatrix * perFrameShaderParameters.nonJitteredPrevViewProjectionMatrix;
            Matrix4x4 inverseReprojectionMatrix = glm::inverse(reprojectionMatrix);

            sl::Constants constants = {};
            constants.cameraViewToClip = *((sl::float4x4*)&perFrameShaderParameters.nonJitteredProjectionMatrix);
            constants.clipToCameraView = *((sl::float4x4*)&perFrameShaderParameters.nonJitteredInvProjectionMatrix);
            //constants.clipToLensClip = {};
            constants.clipToPrevClip = *((sl::float4x4*)&reprojectionMatrix);
            constants.prevClipToClip = *((sl::float4x4*)&inverseReprojectionMatrix);
            constants.jitterOffset = sl::float2(perFrameShaderParameters.cameraJitterOffset.x, perFrameShaderParameters.cameraJitterOffset.y);
            constants.mvecScale = { 1.0f, 1.0f };//sl::float2(perFrameData.data.renderResolution.width, perFrameData.data.renderResolution.height);
            constants.cameraPinholeOffset = { 0.0f, 0.0f };
            constants.cameraPos = sl::float3(perFrameShaderParameters.cameraPosition.x, perFrameShaderParameters.cameraPosition.y, perFrameShaderParameters.cameraPosition.z);
            constants.cameraUp = sl::float3(perFrameShaderParameters.cameraUp.x, perFrameShaderParameters.cameraUp.y, perFrameShaderParameters.cameraUp.z);
            constants.cameraRight = sl::float3(perFrameShaderParameters.cameraRight.x, perFrameShaderParameters.cameraRight.y, perFrameShaderParameters.cameraRight.z);
            constants.cameraFwd = sl::float3(perFrameShaderParameters.cameraForward.x, perFrameShaderParameters.cameraForward.y, perFrameShaderParameters.cameraForward.z);
            constants.cameraNear = perFrameShaderParameters.cameraNearPlane;
            constants.cameraFar = perFrameShaderParameters.cameraFarPlane;
            constants.cameraFOV = perFrameShaderParameters.cameraHalfFovRad * 2.0f;
            constants.cameraAspectRatio = perFrameShaderParameters.cameraAspectRatio;
            constants.motionVectorsInvalidValue = sl::INVALID_FLOAT;
            constants.depthInverted = sl::Boolean::eTrue;
            constants.cameraMotionIncluded = sl::Boolean::eTrue;
            constants.motionVectors3D = sl::Boolean::eFalse;
            constants.reset = perFrameShaderParameters.frameIndex == 0 ? sl::Boolean::eTrue : sl::Boolean::eFalse;
            constants.orthographicProjection = sl::Boolean::eFalse;
            constants.motionVectorsDilated = sl::Boolean::eFalse;
            constants.motionVectorsJittered = sl::Boolean::eFalse;

            sl::Result result = sl::Result::eOk;
            sl::ViewportHandle viewport = 0;
            sl::FrameToken* frameToken = nullptr;
            if (SL_FAILED(result, slGetNewFrameToken(frameToken, &perFrameShaderParameters.frameIndex)))
            {
                LogError(GLogger, std::format("slGetNewFrameToken, error code: {}", (int32)result));
            }
            if (SL_FAILED(result, slSetConstants(constants, *frameToken, viewport)))
            {
                LogError(GLogger, std::format("slSetConstants, error code: {}", (int32)result));
            }
        }

    if (IsDLSSEnabled())
        {
            sl::Result slResult = sl::Result::eOk;
            sl::ViewportHandle slViewport = 0;

            sl::DLSSMode dlssMode = sl::DLSSMode::eOff;
            switch (view.renderSettings.dlssSettings.qualityMode)
            {
            case DLSSQualityMode::Off:
            {
                dlssMode = sl::DLSSMode::eOff;
            } break;
            case DLSSQualityMode::Auto:
            {
                // TODO
                //dlssMode = sl::DLSSMode::eUltraQuality;
                dlssMode = sl::DLSSMode::eMaxQuality;
            } break;
            case DLSSQualityMode::Quality:
            {
                dlssMode = sl::DLSSMode::eMaxQuality;
            } break;
            case DLSSQualityMode::Balanced:
            {
                dlssMode = sl::DLSSMode::eBalanced;
            } break;
            case DLSSQualityMode::Performance:
            {
                dlssMode = sl::DLSSMode::eMaxPerformance;
            } break;
            case DLSSQualityMode::UltraPerformance:
            {
                dlssMode = sl::DLSSMode::eUltraPerformance;
            } break;
            default:
            {
                dlssMode = sl::DLSSMode::eOff;
            } break;
            }

            sl::DLSSOptions dlssOptions = {};
            dlssOptions.mode = dlssMode;
            dlssOptions.outputWidth = targetResolution.width;
            dlssOptions.outputHeight = targetResolution.height;
            dlssOptions.sharpness = 0.0f;
            dlssOptions.preExposure = 1.0f;
            dlssOptions.exposureScale = 1.0f;
            dlssOptions.colorBuffersHDR = sl::Boolean::eTrue;
            dlssOptions.indicatorInvertAxisX = sl::Boolean::eFalse;
            dlssOptions.indicatorInvertAxisY = sl::Boolean::eFalse;
            dlssOptions.dlaaPreset = sl::DLSSPreset::ePresetA;
            dlssOptions.qualityPreset = sl::DLSSPreset::ePresetB;
            dlssOptions.balancedPreset = sl::DLSSPreset::ePresetC;
            dlssOptions.performancePreset = sl::DLSSPreset::ePresetD;
            dlssOptions.ultraPerformancePreset = sl::DLSSPreset::ePresetE;
            if (SL_FAILED(result, slDLSSSetOptions(slViewport, dlssOptions)))
            {
                LogError(GLogger, std::format("slDLSSSetOptions, error code: {}", (int32)result));
            }

            sl::DLSSOptimalSettings dlssOptimalSettins = {};
            if (SL_FAILED(result, slDLSSGetOptimalSettings(dlssOptions, dlssOptimalSettins)))
            {
                LogError(GLogger, std::format("slDLSSGetOptimalSettings, error code: {}", (int32)result));
            }

            renderResolution.width = dlssOptimalSettins.optimalRenderWidth;
            renderResolution.height = dlssOptimalSettins.optimalRenderHeight;

            upscaleRatio = (float)targetResolution.width / (float)renderResolution.width;

            // Dynamic Rendering
            // settings.minRenderSize.x = dlssOptimalSettins.renderWidthMin;
            // settings.minRenderSize.y = dlssOptimalSettins.renderHeightMin;
            // settings.maxRenderSize.x = dlssOptimalSettins.renderWidthMax;
            // settings.maxRenderSize.y = dlssOptimalSettins.renderHeightMax;

            LogInfo(GLogger, std::format("DLSS Mode: {}", (uint32)view.renderSettings.dlssSettings.qualityMode));
            LogInfo(GLogger, std::format("TargetWidth {} TargetHeight {}", targetResolution.width, targetResolution.height));
            LogInfo(GLogger, std::format("RenderWidth {} RenderHeight {}", renderResolution.width, renderResolution.height));
            LogInfo(GLogger, std::format("DynamicMinimumRenderSizeWidth {} DynamicMinimumRenderSizeHeight {}", dlssOptimalSettins.renderWidthMin, dlssOptimalSettins.renderHeightMin));
            LogInfo(GLogger, std::format("DynamicMaximumRenderSizeWidth {} DynamicMaximumRenderSizeHeight {}", dlssOptimalSettins.renderWidthMax, dlssOptimalSettins.renderHeightMax));
        }

    RenderGraphTextureHandle StreamlineDLSS::AddDLSSPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        RenderGraphTextureHandle sceneDepthTexture,
        RenderGraphTextureHandle motionVectorTexture)
    {
        RenderGraphTextureDesc dlssOutputTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolutionX,
            targetResolutionY,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle dlssOutputTexture = renderGraph.CreateTexture(dlssOutputTextureDesc, "DLSSOutputTexture");

        uint32 frameIndex = sceneViewShaderParameters.frameIndex;
        bool reset = (frameIndex == 0) ? true : false;

        renderGraph.AddPass("EvaluateDLSSPass", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);

                dlssOutputTexture = builder.WriteTexture(dlssOutputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.EvaluateDLSS(
                        registry.GetRenderBackendTexture(dlssOutputTexture),
                        registry.GetRenderBackendTexture(sceneColorTexture),
                        registry.GetRenderBackendTexture(sceneDepthTexture),
                        registry.GetRenderBackendTexture(motionVectorTexture),
                        targetResolutionX,
                        targetResolutionY,
                        frameIndex);
                };
            });

        return dlssOutputTexture;
    }
}