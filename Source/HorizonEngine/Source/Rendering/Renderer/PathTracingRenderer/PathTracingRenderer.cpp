#include "PathTracingRenderer.h"
#include "Rendering/Renderer/RasterizationRenderer/AtmosphereRendering.h"

namespace Horizon
{
    PathTracingRenderer::PathTracingRenderer(
        RenderBackend* renderBackend,
        RenderGraphResourcePool* resourcePool,
        ShaderRepository* shaderRepository,
        RendererDefaultResources* defaultResources)
        : renderBackend(renderBackend)
        , resourcePool(resourcePool)
        , shaderCollection(shaderRepository)
        , defaultResources(defaultResources)
        , postProcessingPipeline(renderBackend, resourcePool, shaderRepository, defaultResources)
    {

    }

    PathTracingRenderer::~PathTracingRenderer()
    {
    }

    void PathTracingRenderer::Tick(float deltaTimeInSeconds)
    {
    }

    void PathTracingRenderer::UpdateUniformVariables()
    {
        const SceneView& view = *sceneView;
        const RenderSettings& renderSettings = view.GetRenderSettings();
        const RenderScene* scene = view.GetRenderScene();

        // Setup uniform variables
        {
            uniformVariables.frameIndex = view.frameIndex;
            uniformVariables.frameIndexMod8 = view.frameIndex % 8;

            uniformVariables.deltaTimeInSeconds = view.deltaTimeInSeconds;

            uniformVariables.renderWidth = renderResolution.width;
            uniformVariables.renderHeight = renderResolution.height;
            uniformVariables.targetWidth = targetResolution.width;
            uniformVariables.targetHeight = targetResolution.height;
            uniformVariables.displayWidth = displayResolution.width;
            uniformVariables.displayHeight = displayResolution.height;

            uniformVariables.renderResolution = Vector4f(1.0f * renderResolution.width, 1.0f * renderResolution.height, 1.0f / renderResolution.width, 1.0f / renderResolution.height);
            uniformVariables.targetResolution = Vector4f(1.0f * targetResolution.width, 1.0f * targetResolution.height, 1.0f / targetResolution.width, 1.0f / targetResolution.height);
            uniformVariables.displayResolution = Vector4f(1.0f * displayResolution.width, 1.0f * displayResolution.height, 1.0f / displayResolution.width, 1.0f / displayResolution.height);

            uniformVariables.cameraPosition = view.cameraPosition;
            uniformVariables.cameraUpVector = view.cameraUpVector;
            uniformVariables.cameraRightVector = view.cameraRightVector;
            uniformVariables.cameraForwardVector = view.cameraForwardVector;
            uniformVariables.halfFovInRadians = view.verticalFOV * 0.5f;
            uniformVariables.tanHalfFovY = std::tan(view.verticalFOV * 0.5f);
            uniformVariables.aspectRatio = view.aspectRatio;
            uniformVariables.nearClippingPlane = view.nearClippingPlane;
            uniformVariables.farClippingPlane = view.farClippingPlane;

            uniformVariables.viewFrustum[0] = Vector4f(view.viewFrustum.planes[0].normal, view.viewFrustum.planes[0].distance);
            uniformVariables.viewFrustum[1] = Vector4f(view.viewFrustum.planes[1].normal, view.viewFrustum.planes[1].distance);
            uniformVariables.viewFrustum[2] = Vector4f(view.viewFrustum.planes[2].normal, view.viewFrustum.planes[2].distance);
            uniformVariables.viewFrustum[3] = Vector4f(view.viewFrustum.planes[3].normal, view.viewFrustum.planes[3].distance);
            uniformVariables.viewFrustum[4] = Vector4f(view.viewFrustum.planes[4].normal, view.viewFrustum.planes[4].distance);
            uniformVariables.viewFrustum[5] = Vector4f(view.viewFrustum.planes[5].normal, view.viewFrustum.planes[5].distance);

            uniformVariables.cameraJitterOffset = cameraJitterOffset;
            uniformVariables.previousCameraJitterOffset = historicalData.cameraJitterOffset;
            uniformVariables.motionVectorJitterCancellation = (historicalData.cameraJitterOffset - cameraJitterOffset) * Vector2f(1.0f / renderResolution.width, 1.0f / renderResolution.height);

            uniformVariables.previousCameraPosition = historicalData.cameraPosition;

            uniformVariables.viewSpaceDepthToNDCSpaceDepthTransform = view.transformations.viewSpaceDepthToNDCSpaceDepthTransform;

            uniformVariables.worldToViewMatrix = view.transformations.worldToViewMatrix;
            uniformVariables.viewToWorldMatrix = view.transformations.viewToWorldMatrix;
            uniformVariables.viewToClipMatrix = view.transformations.viewToClipMatrix;
            uniformVariables.clipToViewMatrix = view.transformations.clipToViewMatrix;
            uniformVariables.worldToClipMatrix = view.transformations.worldToClipMatrix;
            uniformVariables.clipToWorldMatrix = view.transformations.clipToWorldMatrix;
            uniformVariables.nonJitteredWorldToClipMatrix = view.transformations.nonJitteredWorldToClipMatrix;
            uniformVariables.nonJitteredClipToWorldMatrix = view.transformations.nonJitteredClipToWorldMatrix;

            uniformVariables.previousWorldToViewMatrix = historicalData.transformations.worldToViewMatrix;
            uniformVariables.previousViewToWorldMatrix = historicalData.transformations.viewToWorldMatrix;
            uniformVariables.previousViewToClipMatrix = historicalData.transformations.viewToClipMatrix;
            uniformVariables.previousClipToViewMatrix = historicalData.transformations.clipToViewMatrix;
            uniformVariables.previousWorldToClipMatrix = historicalData.transformations.worldToClipMatrix;
            uniformVariables.previousClipToWorldMatrix = historicalData.transformations.clipToWorldMatrix;
            uniformVariables.previousNonJitteredWorldToClipMatrix = historicalData.transformations.nonJitteredWorldToClipMatrix;

            // TODO: Precision loss
            reprojectionMatrix = uniformVariables.previousNonJitteredWorldToClipMatrix * uniformVariables.nonJitteredClipToWorldMatrix;
            inverseReprojectionMatrix = glm::inverse(reprojectionMatrix);

            uniformVariables.currentClipToPreviousClipMatrix = reprojectionMatrix;
            uniformVariables.previousClipToCurrentClipMatrix = inverseReprojectionMatrix;

            uniformVariables.materialTextureMipLodBias = materialTextureMipLodBias;

            uniformVariables.preExposure = preExposure;
            uniformVariables.inversePreExposure = 1.0f / preExposure;
            uniformVariables.preExposureCorrection = preExposure / historicalData.preExposure;

            uniformVariables.motionVectorScale = Vector2f(float(renderResolution.width), float(renderResolution.height));

            uniformVariables.samplesPerPixel = rendererSettings.samplesPerPixel;
            uniformVariables.maxBounces = rendererSettings.maxBounces;

            uniformVariables.indirectLightingMultiplier = Vector3f(1.0f, 1.0f, 1.0f);

            if (scene != nullptr)
            {
                if (scene->HasAtmosphericLight())
                {
                    const LightRenderObject* atmosphericLight = scene->GetAtmosphericLight();
                    const float halfApexAngle = atmosphericLight->GetHalfApexAngleInRadians();
                    const float cosHalfApexAngle = std::cos(halfApexAngle);
                    const float solidAngle = 2.0f * M_PI * (1.0f - cosHalfApexAngle); // https://en.wikipedia.org/wiki/Solid_angle

                    const Vector3f atmosphericLightIlluminance = atmosphericLight->GetPhysicalLightColor();
                    const Vector3f atmosphericLightDiskLuminance = atmosphericLight->GetAtmosphericLightDiskColorFactor() * atmosphericLightIlluminance / solidAngle; // approximation

                    uniformVariables.atmosphericLightDirection = atmosphericLight->GetDirection();
                    uniformVariables.atmosphericLightDiskLuminance = atmosphericLightDiskLuminance;
                    uniformVariables.atmosphericLightDiskCosHalfApexAngle = cosHalfApexAngle;
                    uniformVariables.atmosphericLightOuterSpaceIlluminance = atmosphericLightIlluminance;
                }
                else
                {
                    uniformVariables.atmosphericLightDirection = DefaultLightDirection;
                    uniformVariables.atmosphericLightDiskLuminance = Vector3f(0.0f, 0.0f, 0.0f);
                    uniformVariables.atmosphericLightDiskCosHalfApexAngle = 1.0f;
                    uniformVariables.atmosphericLightOuterSpaceIlluminance = Vector3f(0.0f, 0.0f, 0.0f);
                }

                //if (renderFeatures.enableSkyAtmosphereRendering)
                {
                    const SkyAtmosphereRenderObject& skyAtmosphere = *scene->GetActiveSkyAtmosphere();

                    SkyAtmosphereShaderParameters skyAtmosphereShaderParameters = {};
                    SetupSkyAtmosphereShaderParameters(skyAtmosphereShaderParameters, skyAtmosphere);

                    SkyAtmosphereViewRelatedParameters skyAtmosphereViewRelatedParameters =
                    {
                        .skyViewLutReferential = IdentityMatrix3x3f,
                    };
                    SetupSkyAtmosphereViewRelatedParameters(skyAtmosphereViewRelatedParameters, skyAtmosphere, view.cameraPosition, view.cameraForwardVector);

                    uniformVariables.skyAtmosphereTransmittanceLutSize = skyAtmosphereShaderParameters.transmittanceLutSize;
                    uniformVariables.skyAtmosphereMultipleScatteringLutSize = skyAtmosphereShaderParameters.multipleScatteringLutSize;
                    uniformVariables.skyAtmosphereSkyViewLutSize = skyAtmosphereShaderParameters.skyViewLutSize;
                    uniformVariables.skyAtmosphereAerialPerspectiveVolumeSize = skyAtmosphereShaderParameters.aerialPerspectiveVolumeSize;
                    uniformVariables.skyAtmosphereTransmittanceLutSampleCount = skyAtmosphereShaderParameters.transmittanceLutSampleCount;
                    uniformVariables.skyAtmosphereMultipleScatteringLutSampleCount = skyAtmosphereShaderParameters.multipleScatteringLutSampleCount;
                    uniformVariables.skyAtmosphereRayMarchingMinSampleCount = skyAtmosphereShaderParameters.rayMarchingMinSampleCount;
                    uniformVariables.skyAtmosphereRayMarchingMaxSampleCount = skyAtmosphereShaderParameters.rayMarchingMaxSampleCount;
                    uniformVariables.skyAtmosphereBottomRadiusInKilometers = skyAtmosphereShaderParameters.bottomRadius;
                    uniformVariables.skyAtmosphereTopRadiusInKilometers = skyAtmosphereShaderParameters.topRadius;
                    uniformVariables.skyAtmosphereGroundAlbedo = skyAtmosphereShaderParameters.groundAlbedo;
                    uniformVariables.skyAtmosphereRayleighScattering = skyAtmosphereShaderParameters.rayleighScattering;
                    uniformVariables.skyAtmosphereRayleighDensityExpScale = skyAtmosphereShaderParameters.rayleighDensityExpScale;
                    uniformVariables.skyAtmosphereMieScattering = skyAtmosphereShaderParameters.mieScattering;
                    uniformVariables.skyAtmosphereMieAbsorption = skyAtmosphereShaderParameters.mieAbsorption;
                    uniformVariables.skyAtmosphereMieExtinction = skyAtmosphereShaderParameters.mieExtinction;
                    uniformVariables.skyAtmosphereMiePhaseG = skyAtmosphereShaderParameters.miePhaseG;
                    uniformVariables.skyAtmosphereMieDensityExpScale = skyAtmosphereShaderParameters.mieDensityExpScale;
                    uniformVariables.skyAtmosphereAbsorptionDensity0LayerWidth = skyAtmosphereShaderParameters.absorptionDensity0LayerWidth;
                    uniformVariables.skyAtmosphereAbsorptionDensity0ConstantTerm = skyAtmosphereShaderParameters.absorptionDensity0ConstantTerm;
                    uniformVariables.skyAtmosphereAbsorptionDensity0LinearTerm = skyAtmosphereShaderParameters.absorptionDensity0LinearTerm;
                    uniformVariables.skyAtmosphereAbsorptionDensity1ConstantTerm = skyAtmosphereShaderParameters.absorptionDensity1ConstantTerm;
                    uniformVariables.skyAtmosphereAbsorptionDensity1LinearTerm = skyAtmosphereShaderParameters.absorptionDensity1LinearTerm;
                    uniformVariables.skyAtmosphereAbsorptionExtinction = skyAtmosphereShaderParameters.absorptionExtinction;
                    uniformVariables.skyAtmosphereSkyLuminanceFactor = skyAtmosphere.GetSkyLuminanceFactor();
                    uniformVariables.skyAtmosphereSkyViewLutReferential = skyAtmosphereViewRelatedParameters.skyViewLutReferential;
                }
            }

            // TODO: Move post processing settings form RasterizationRendererUniformVariables to other place
            {
                uniformVariables.motionBlurIntensity = finalPostProcessingSettings.motionBlurIntensity;
                uniformVariables.motionBlurMaxVelocityLengthInPixels = finalPostProcessingSettings.motionBlurMaxVelocityLength / 100.0f * std::max(targetResolution.width, targetResolution.height);

                uniformVariables.autoExposureExposureCompensation = finalPostProcessingSettings.autoExposureExposureCompensation;
                uniformVariables.autoExposureMinExposureValue = finalPostProcessingSettings.autoExposureMinExposureValue;
                uniformVariables.autoExposureMaxExposureValue = finalPostProcessingSettings.autoExposureMaxExposureValue;
                uniformVariables.autoExposureSpeedDarkToBright = finalPostProcessingSettings.autoExposureSpeedDarkToBright;
                uniformVariables.autoExposureSpeedBrightToDark = finalPostProcessingSettings.autoExposureSpeedBrightToDark;
                uniformVariables.autoExposureHistogramLowerPercentage = finalPostProcessingSettings.autoExposureHistogramLowerPercentage;
                uniformVariables.autoExposureHistogramHigherPercentage = finalPostProcessingSettings.autoExposureHistogramHigherPercentage / 100.0f;
                uniformVariables.autoExposureHistogramMinEV100 = finalPostProcessingSettings.autoExposureHistogramMinEV100;
                uniformVariables.autoExposureHistogramMaxEV100 = finalPostProcessingSettings.autoExposureHistogramMaxEV100;
                uniformVariables.autoExposureUseTargetExposure = 0.0f;//(view.NeedToBeReset() || !renderFeatures.enableAutoExposure) ? 1.0f : 0.0f; // TODO: forceUseTargetExposure;
                uniformVariables.autoExposureMinimumLuminance = std::exp2(finalPostProcessingSettings.autoExposureHistogramMinEV100);

                uniformVariables.bloomIntensity = finalPostProcessingSettings.bloomIntensity;
                uniformVariables.bloomRadius = finalPostProcessingSettings.bloomRadius;

                uniformVariables.lensFlareIntensity = finalPostProcessingSettings.lensFlareIntensity;
                uniformVariables.lensFlareHaloIntensity = finalPostProcessingSettings.lensFlareHaloIntensity;
                uniformVariables.lensFlareHaloWidth = finalPostProcessingSettings.lensFlareHaloWidth;
                uniformVariables.lensFlareHaloChromaticAberrationOffset = finalPostProcessingSettings.lensFlareHaloChromaticAberrationOffset;

                uniformVariables.chromaticAberrationIntensity = finalPostProcessingSettings.chromaticAberrationIntensity;
                uniformVariables.chromaticAberrationOffset = finalPostProcessingSettings.chromaticAberrationOffset;

                uniformVariables.whiteBalance = finalPostProcessingSettings.whiteBalance;

                uniformVariables.vignetteIntensity = finalPostProcessingSettings.vignetteIntensity;

                //uniformVariables.toneMappingOperator = finalPostProcessingSettings.toneMappingOperator;

                //uniformVariables.colorCorrectionSaturation = finalPostProcessingSettings.colorCorrectionSaturation;
                //uniformVariables.colorCorrectionContrast = finalPostProcessingSettings.colorCorrectionContrast;
                //uniformVariables.colorCorrectionGamma = finalPostProcessingSettings.colorCorrectionGamma;
                //uniformVariables.colorCorrectionGain = finalPostProcessingSettings.colorCorrectionGain;
                //uniformVariables.colorCorrectionOffset = finalPostProcessingSettings.colorCorrectionOffset;
            }
        }

    }

    void PathTracingRenderer::InitializeSceneView(SceneView* v)
    {
        sceneView = v;
        SceneView& view = *sceneView;

        if (view.renderSettings.renderMode == RenderMode::RealTimePathTracing)
        {
            rendererSettings = view.renderSettings.realTimePathTracingSettings;
        }
        else if (view.renderSettings.renderMode == RenderMode::ReferencePathTracing)
        {
            rendererSettings = view.renderSettings.referencePathTracingSettings;
        }

        finalPostProcessingSettings = view.renderSettings.postProcessingSettings;

        {
            renderResolutionPercentage = 1.0f;

            renderResolution = Extent2D(view.targetWidth, view.targetHeight);
            targetResolution = Extent2D(view.targetWidth, view.targetHeight);
            displayResolution = Extent2D(view.targetWidth, view.targetHeight);
        }

        view.transformations.Finalize();

        UpdateUniformVariables();
    }

    void PathTracingRenderer::Render(RenderGraph& renderGraph)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "PathTracingRenderer");

        const SceneView& view = *sceneView;

        if (!perFrameConstantBuffers[currentPerFrameDataBufferIndex])
        {
            RenderBackendBufferDescription perFrameConstantUploadBufferDesc = RenderBackendBufferDescription::CreateUpload(sizeof(PathTracingRendererUniformVariables));
            perFrameConstantUploadBuffers[currentPerFrameDataBufferIndex] = renderBackend->CreateBuffer(&perFrameConstantUploadBufferDesc, nullptr, "PerFrameConstantUploadBuffer");
            RenderBackendBufferDescription perFrameConstantBufferDesc = RenderBackendBufferDescription::CreateStructured(sizeof(PathTracingRendererUniformVariables), 1);
            perFrameConstantBuffers[currentPerFrameDataBufferIndex] = renderBackend->CreateBuffer(&perFrameConstantBufferDesc, nullptr, "PerFrameConstantBuffer");
        }
        renderBackend->UpdateBuffer(perFrameConstantUploadBuffers[currentPerFrameDataBufferIndex], 0, &uniformVariables, sizeof(PathTracingRendererUniformVariables));

        RenderBackendBufferHandle perFrameConstantUploadBuffer = perFrameConstantUploadBuffers[currentPerFrameDataBufferIndex];
        RenderBackendBufferHandle perFrameConstantBuffer = perFrameConstantBuffers[currentPerFrameDataBufferIndex];

        currentPerFrameConstantBuffer = perFrameConstantBuffers[currentPerFrameDataBufferIndex];

        renderGraph.AddPass(
            std::format("UpdatePerFrameConstants"),
            RenderGraphPassFlags::Copy,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.CopyBuffer(
                        perFrameConstantUploadBuffer,
                        0,
                        perFrameConstantBuffer,
                        0,
                        sizeof(PathTracingRendererUniformVariables));

                    RenderBackendBarrier transitionAfter = RenderBackendBarrier(perFrameConstantBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::ShaderResource);
                    commandList.Barriers(&transitionAfter, 1);
                };
            });

        PathTracingRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Create<PathTracingRendererIntermediateResources>();

        RenderBackendTextureClearValue clearColor = RenderBackendTextureClearValue::Black;
        RenderGraphTextureDescription colorTextureDescription = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget,
            clearColor,
            1,
            1,
            RenderBackendResourceState::UnorderedAccess);
        intermediateResources.colorTexture = renderGraph.CreateTexture(colorTextureDescription, "PathTracingColorTexture");

        RenderGraphTextureDescription depthTextureDescription = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        intermediateResources.depthTexture = renderGraph.CreateTexture(depthTextureDescription, "PathTracingDepthTexture");

        intermediateResources.environmentMapTexture = renderGraph.ImportExternalTexture(view.scene->skyLights[0]->environmentMapTexture, "SkyLightTexture");

        DispatchPathTracing(renderGraph, view);

        PostProcessingPipelineInputs postProcessingPipelineInputs;
        postProcessingPipelineInputs.colorTexture = intermediateResources.colorTexture;
        postProcessingPipelineInputs.depthTexture = intermediateResources.depthTexture;
        postProcessingPipelineInputs.motionVectorTexture = RenderGraphTextureHandle::Null;
        postProcessingPipelineInputs.renderResolution = renderResolution;
        postProcessingPipelineInputs.targetResolution = targetResolution;
        postProcessingPipelineInputs.perFrameConstantBuffer = currentPerFrameConstantBuffer;
        postProcessingPipelineInputs.postProcessingSettings = finalPostProcessingSettings;
        postProcessingPipelineInputs.featureFlags.enableDepthOfField = false;
        postProcessingPipelineInputs.featureFlags.enableMotionBlur = false;//finalPostProcessingSettings.motionBlurIntensity > 0.0f;
        postProcessingPipelineInputs.featureFlags.enableAutoExposure = (finalPostProcessingSettings.exposureMethod == ExposureMethod::AutoExposure);
        postProcessingPipelineInputs.featureFlags.enableBilateralGridLocalToneMapping = (finalPostProcessingSettings.localToneMappingMethod == LocalToneMappingMethod::BilateralGrid);
        postProcessingPipelineInputs.featureFlags.enableExposureFusionLocalToneMapping = (finalPostProcessingSettings.localToneMappingMethod == LocalToneMappingMethod::ExposureFusion);
        postProcessingPipelineInputs.featureFlags.enableGaussianBloom = finalPostProcessingSettings.bloomIntensity > 0.0f;
        postProcessingPipelineInputs.featureFlags.enableLensFlare = finalPostProcessingSettings.lensFlareIntensity > 0.0f;
        postProcessingPipelineInputs.temporalSuperSamplingInterface = nullptr;
        postProcessingPipelineInputs.viewNeedsReset = view.NeedToBeReset();
        postProcessingPipeline.Execute(renderGraph, view, postProcessingPipelineInputs);

        historicalData.preExposure = preExposure;
        historicalData.cameraJitterOffset = cameraJitterOffset;
        historicalData.cameraPosition = view.GetCameraPosition();
        historicalData.transformations = view.GetCameraTransformations();

        currentPerFrameDataBufferIndex = (currentPerFrameDataBufferIndex + 1) % MaxNumFramesInFlight;
    }
}