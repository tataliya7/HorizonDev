#include "RasterizationRenderer.h"
#include "AtmosphereRendering.h"
#include "TemporalSuperSampling.h"
#include "StreamlineModule.h"

#include <optick.h>

import FidelityFX.FSR2;
import FidelityFX.FSR3;

namespace Horizon
{
    RasterizationRenderer::RasterizationRenderer(
        RenderBackend* renderBackend,
        RenderGraphResourcePool* resourcePool,
        ShaderRepository* shaderRepository,
        RendererDefaultResources* defaultResources)
        : renderBackend(renderBackend)
        , resourcePool(resourcePool)
        , shaderCollection(shaderRepository)
        , defaultResources(defaultResources)
        , temporalSuperSamplingInterface(nullptr)
    {
        //RenderBackendBufferDesc perFrameConstantBufferDesc = RenderBackendBufferDesc::CreateStructured(sizeof(RasterizationRendererUniformVariables), 1);
        //RenderBackendBufferDesc perFrameConstantUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(sizeof(RasterizationRendererUniformVariables));
        //for (uint32 index = 0; index < MaxNumFramesInFlight; index++)
        //{
        //    perFrameConstantBuffers[index] = renderBackend->CreateBuffer(&perFrameConstantBufferDesc, nullptr, "PerFrameConstantBuffer");
        //    perFrameConstantUploadBuffers[index] = renderBackend->CreateBuffer(&perFrameConstantUploadBufferDesc, nullptr, "PerFrameConstantBufferUpload");
        //}

        AutoExposureData defaultAutoExposureData;
        RenderBackendBufferDescription autoExposureReadbackBufferDesc = RenderBackendBufferDescription::CreateReadback(sizeof(AutoExposureData));
        for (uint32 index = 0; index < AutoExposureReadbackBufferCount; index++)
        {
            RenderBackendBufferHandle autoExposureReadbackBuffer = renderBackend->CreateBuffer(&autoExposureReadbackBufferDesc, &defaultAutoExposureData, "AutoExposureReadBackBuffer");
            autoExposureReadbackBuffers[index] = resourcePool->CacheBuffer(autoExposureReadbackBuffer, autoExposureReadbackBufferDesc, "AutoExposureReadBackBuffer");
        }

        ResetHistoryFrame();

        virtualShadowMapManager = new VirtualShadowMapManager();
    }

    RasterizationRenderer::~RasterizationRenderer()
    {

    }

    void RasterizationRenderer::Tick(float deltaTimeInSeconds)
    {
        debugDrawLinesVertices.clear();
    }

    RenderBackendBufferHandle RasterizationRenderer::GetCurrentPerFrameConstantBuffer() const
    {
        return currentPerFrameConstantBuffer;
    }

    void RasterizationRenderer::ResetHistoryFrame()
    {
        historyFrame.cameraPosition = Vector3f(0.0f, 0.0f, 0.0f);
        historyFrame.cameraJitterOffset = Vector2f(0.0f, 0.0f);
        historyFrame.transformations.Reset();
        historyFrame.preExposure = 1.0f;

        AutoExposureData defaultAutoExposureData;
        RenderBackendBufferDescription autoExposureBufferDesc = RenderBackendBufferDescription::Create(sizeof(AutoExposureData), 1, RenderBackendBufferCreateFlags::ShaderResource | RenderBackendBufferCreateFlags::UnorderedAccess);
        RenderBackendBufferHandle autoExposureBuffer = renderBackend->CreateBuffer(&autoExposureBufferDesc, &defaultAutoExposureData, "AutoExposureBuffer");
        historyFrame.autoExposureBuffer = resourcePool->CacheBuffer(autoExposureBuffer, autoExposureBufferDesc, "AutoExposureBuffer");

        historyFrame.volumetricFogLightScatteringTexture = nullptr;
        historyFrame.temporalSuperSamplingOutputTexture = nullptr;
    }

    void RasterizationRenderer::InitializeSceneView(SceneView* v)
    {
        OPTICK_EVENT();

        debugVisualizationCallback = {};

        sceneView = v;
        SceneView& view = *sceneView;
        rendererSettings = view.renderSettings.rasterRenderingSettings;

        // TODO: initialize buffer
        UpdateAutoExposureDataFromReadbackBuffer();

        // TODO
        if (view.frameIndex >= 3)
        {
            preExposure = autoExposureData.adaptedExposure;
        }
        if (rendererSettings.enableFixedPreExposure)
        {
            preExposure = rendererSettings.fixedPreExposure;
        }

        renderResolutionPercentage = 1.0f;

        if (temporalSuperSamplingInterface == nullptr)
        {
            if (rendererSettings.superSamplingSettings.superSamplingTechnique == SuperSamplingTechnique::FSR2)
            {
                temporalSuperSamplingInterface = FidelityFXSuperResolution2Create(renderBackend);
            }
            else if (rendererSettings.superSamplingSettings.superSamplingTechnique == SuperSamplingTechnique::DLSS)
            {
                temporalSuperSamplingInterface = StreamlineDLSSSuperResolutionCreate(renderBackend);
            }
        }

        if (rendererSettings.superSamplingSettings.superSamplingTechnique == SuperSamplingTechnique::None)
        {
            // TODO: destroy resources

            temporalSuperSamplingInterface = nullptr;
        }

        renderResolution = Extent2D(view.targetWidth, view.targetHeight);
        targetResolution = Extent2D(view.targetWidth, view.targetHeight);
        displayResolution = Extent2D(view.targetWidth, view.targetHeight);

        cameraJitterOffset = Vector2f(0.0f, 0.0f);

        if (temporalSuperSamplingInterface != nullptr)
        {
            TemporalSuperSamplingOptions tssOptions = {};
            tssOptions.qualityMode = rendererSettings.superSamplingSettings.qualityMode;
            tssOptions.desiredRenderResolutionPercentage = rendererSettings.superSamplingSettings.desiredRenderResolutionPercentage;
            tssOptions.outputWidth = targetResolution.width;
            tssOptions.outputHeight = targetResolution.height;
            tssOptions.preExposure = preExposure;
            temporalSuperSamplingInterface->SetOptions(tssOptions);

            TemporalSuperSamplingOptimalSettings optimalSettings = temporalSuperSamplingInterface->GetOptimalSettings();

            renderResolution.width = optimalSettings.optimalRenderWidth;
            renderResolution.height = optimalSettings.optimalRenderHeight;
            renderResolutionPercentage = optimalSettings.optimalRenderResolutionPercentage;

            uint32 jitterPhaseCount = temporalSuperSamplingInterface->GetJitterPhaseCount(renderResolution.width, targetResolution.width);
            cameraJitterOffset = temporalSuperSamplingInterface->GetJitterOffset(view.frameIndex, jitterPhaseCount);

            view.transformations.ApplyJitterOffset(cameraJitterOffset, renderResolution.width, renderResolution.height);
        }

        view.transformations.Finalize();

        materialTextureMipLodBias = 0.0f;

        //if (IsAutoMaterialTextureMipLodBiasEnabled())
        {
            materialTextureMipLodBias = std::log2f(float(renderResolution.width) / float(targetResolution.width));
        }

        // TODO: Override overrideMaterialTextureMipLodBias
        //if (overrideMaterialTextureMipLodBias)
        //{
        //
        //}

        /*

        Vector2f previousCameraJitterOffset = perFrameShaderParameters.cameraJitterOffset;
        if (view.frameIndex == 0)
        {
            previousCameraJitterOffset = cameraJitterOffset;
        }

        Vector3f previousCameraPosition = perFrameShaderParameters.cameraPosition;

        enableFixedExposure = settings.exposureMethod == ExposureMethod::FixedExposure;*/

        //float prevpreExposure = perFrameShaderParameters.preExposure;
        //if (view.frameIndex == 0)
        //{
        //    prevpreExposure = 1.0f;
        //}

        //float preExposure = 1.0f;
        //if (view. != 0)
        //{
        //    preExposure = preExposure;
        //}
        //if ()
        //{
        //    preExposure = view.renderSettings.fixedPreExposure;
        //}

        finalPostProcessingSettings = rendererSettings.postProcessingSettings;

        const RenderScene* scene = view.GetRenderScene();

        renderFeatures.enableTemporalSuperSampling = temporalSuperSamplingInterface != nullptr;
        renderFeatures.enableSkyAtmosphereRendering =
            scene != nullptr &&
            scene->HasAtmosphericLight() &&
            scene->HasActiveSkyAtmosphere();
        renderFeatures.enableVolumetricFog = false;
        renderFeatures.enableScreenSpaceShadows = true;
        renderFeatures.enableScreenSpaceReflections = rendererSettings.reflectionsTechnique == RasterizationRendererReflectionsTechnique::ScreenSpaceReflections;
        renderFeatures.enableScreenSpaceAmbientOcclusion = rendererSettings.ambientOcclusionTechnique == RasterizationRendererAmbientOcclusionTechnique::GroundTruthAmbientOcclusion;
        renderFeatures.enableScreenSpaceLightShafts = true;
        renderFeatures.enableDepthOfField = false;// finalPostProcessingSettings.depthOfFieldScale > 0.0f;
        renderFeatures.enableMotionBlur = finalPostProcessingSettings.motionBlurIntensity > 0.0f;
        renderFeatures.enableAutoExposure = (finalPostProcessingSettings.exposureMethod == ExposureMethod::AutoExposure);
        renderFeatures.enableGaussianBloom = finalPostProcessingSettings.bloomIntensity > 0.0f;
        renderFeatures.enableLensFlare = finalPostProcessingSettings.lensFlareIntensity > 0.0f;
        renderFeatures.enableConvolutionBloom = false;
        renderFeatures.enableEditorSelectionOutline = false;
        renderFeatures.enableSubsurfaceScattering = false;
        renderFeatures.enableBilateralGridLocalToneMapping = (finalPostProcessingSettings.localToneMappingMethod == LocalToneMappingMethod::BilateralGrid);
        renderFeatures.enableExposureFusionLocalToneMapping = (finalPostProcessingSettings.localToneMappingMethod == LocalToneMappingMethod::ExposureFusion);

        if (!renderFeatures.enableAutoExposure)
        {
            finalPostProcessingSettings.autoExposureMinExposureValue = finalPostProcessingSettings.fixedExposureValue;
            finalPostProcessingSettings.autoExposureMaxExposureValue = finalPostProcessingSettings.fixedExposureValue;
        }

        UpdatePerFrameDataBuffer();

        if (temporalSuperSamplingInterface != nullptr)
        {
            TemporalSuperSamplingConstants tssConstants = {};
            tssConstants.reset = false; // TODO
            tssConstants.frameIndex = view.frameIndex;
            tssConstants.sharpness = 0.0f;
            tssConstants.deltaTime = view.deltaTimeInSeconds * 1000.0f;
            tssConstants.preExposure = preExposure;
            tssConstants.renderWidth = renderResolution.width;
            tssConstants.renderHeight = renderResolution.height;
            tssConstants.jitterOffset = cameraJitterOffset;
            tssConstants.motionVectorScale = Vector2f(1.0f, 1.0f);
            tssConstants.cameraNearClippingPlane = view.nearClippingPlane;
            tssConstants.cameraFarClippingPlane = view.farClippingPlane;
            tssConstants.cameraFovAngleVertical = view.verticalFOV;
            tssConstants.cameraAspectRatio = view.aspectRatio;
            tssConstants.cameraPosition = view.cameraPosition;
            tssConstants.cameraUpVector = view.cameraUpVector;
            tssConstants.cameraRightVector = view.cameraRightVector;
            tssConstants.cameraForwardVector = view.cameraForwardVector;
            tssConstants.cameraForwardVector = view.cameraForwardVector;
            tssConstants.cameraForwardVector = view.cameraForwardVector;
            tssConstants.nonJitteredViewToClipMatrix = view.transformations.nonJitteredViewToClipMatrix;
            tssConstants.nonJitteredClipToViewMatrix = view.transformations.nonJitteredClipToViewMatrix;
            tssConstants.reprojectionMatrix = reprojectionMatrix;
            tssConstants.inverseReprojectionMatrix = inverseReprojectionMatrix;

            temporalSuperSamplingInterface->SetConstants(tssConstants);
        }

        DispatchDynamicShadowSetupJobs();

        SetupGeometryPasses();
    }

    void RasterizationRenderer::GatherVisibleLights()
    {
        const SceneView& view = *sceneView;
        const RenderScene* scene = view.GetRenderScene();
        const RenderSettings& renderSettings = view.GetRenderSettings();

        visibleLocalLights.reserve(scene->lights.size());

        for (LightRenderObject* light : scene->lights)
        {
            if (light->IsDistantLight())
            {

            }

            // @todo When considering accurate global illumination, light sources outside the view frustum also contribute to the final scene.
            if (light->IsLocalLight() && (renderSettings.renderMode != RenderMode::ReferencePathTracing))
            {
                const Sphere lightBoundingSphere = light->GetBoundingSphere();
                const bool frustumCullingTest = FrustumSphereIntersectionTest(view.viewFrustum, lightBoundingSphere);

                const float distanceSquared = Math::DotProduct(lightBoundingSphere.position, view.cameraPosition);
                const float cullDistance = light->GetCullDistance();

                const bool distanceCullingTest = distanceSquared < cullDistance * cullDistance;

                const bool visible = frustumCullingTest && distanceCullingTest;

                if (visible)
                {
                    //visibleLocalLights.push_back();
                }
            }
        }
    }

    void RasterizationRenderer::DispatchDynamicShadowSetupJobs()
    {
        virtualShadowMapManager->Clear();
        CreateDynamicShadowData();
        // GatherDynamicShadowCasters();
    }

    void RasterizationRenderer::CreateDynamicShadowData()
    {
        const SceneView& view = *sceneView;
        const RenderScene* scene = view.GetRenderScene();
        const RenderSettings& renderSettings = view.GetRenderSettings();

        for (LightRenderObject* light : scene->lights)
        {
            const bool castDynamicShadows = light->castDynamicShadows;
            if (!castDynamicShadows)
            {
                continue;
            }

            // todo: visible
            const bool useCascadedShadowMap = (rendererSettings.shadowsTechnique == RasterizationRendererShadowsTechnique::ShadowMaps) && (light->lightType == LightType::DistantLight);
            const bool useVirtualShadowMap = (rendererSettings.shadowsTechnique == RasterizationRendererShadowsTechnique::VirtualShadowMaps) && (light->lightType == LightType::DistantLight);

            if (useCascadedShadowMap)
            {
                SetupViewDependentCascadedShadowMapRenderDataForLight(cascadedShadowMapRenderData, view, *light);
            }
            else if (useVirtualShadowMap)
            {
                virtualShadowMapManager->CreateVirtualShadowMapClipmap(view, *light);
            }
        }
    }

    void RasterizationRenderer::UpdatePerFrameDataBuffer()
    {
        const SceneView& view = *sceneView;
        const RenderSettings& renderSettings = view.GetRenderSettings();
        const RenderScene* scene = view.GetRenderScene();

#if 0
        for (auto& mesh : scene->meshes)
        {
            for (auto t : mesh->jointBindTransforms)
            {
                t = mesh->localToWorldMatrix * t;
                Vector3f translation; Quaternion rotation; Vector3f scale;
                Math::DecomposeTransformationMatrix(t, translation, rotation, scale);
                DrawSphere(translation, 0.1f, Vector4f(1, 0, 0, 1));
            }
        }
#endif
        // Setup uniform variables
        {
            perFrameShaderParameters.frameIndex = view.frameIndex;
            perFrameShaderParameters.frameIndexMod8 = view.frameIndex % 8;

            perFrameShaderParameters.deltaTimeInSeconds = view.deltaTimeInSeconds;

            perFrameShaderParameters.renderWidth = renderResolution.width;
            perFrameShaderParameters.renderHeight = renderResolution.height;
            perFrameShaderParameters.targetWidth = targetResolution.width;
            perFrameShaderParameters.targetHeight = targetResolution.height;
            perFrameShaderParameters.displayWidth = displayResolution.width;
            perFrameShaderParameters.displayHeight = displayResolution.height;

            perFrameShaderParameters.renderResolution = Vector4f(1.0f * renderResolution.width, 1.0f * renderResolution.height, 1.0f / renderResolution.width, 1.0f / renderResolution.height);
            perFrameShaderParameters.targetResolution = Vector4f(1.0f * targetResolution.width, 1.0f * targetResolution.height, 1.0f / targetResolution.width, 1.0f / targetResolution.height);
            perFrameShaderParameters.displayResolution = Vector4f(1.0f * displayResolution.width, 1.0f * displayResolution.height, 1.0f / displayResolution.width, 1.0f / displayResolution.height);

            perFrameShaderParameters.cameraPosition = view.cameraPosition;
            perFrameShaderParameters.cameraUpVector = view.cameraUpVector;
            perFrameShaderParameters.cameraRightVector = view.cameraRightVector;
            perFrameShaderParameters.cameraForwardVector = view.cameraForwardVector;
            perFrameShaderParameters.halfFovInRadians = view.verticalFOV * 0.5f;
            perFrameShaderParameters.tanHalfFovY = std::tan(view.verticalFOV * 0.5f);
            perFrameShaderParameters.aspectRatio = view.aspectRatio;
            perFrameShaderParameters.nearClippingPlane = view.nearClippingPlane;
            perFrameShaderParameters.farClippingPlane = view.farClippingPlane;

            perFrameShaderParameters.viewFrustum[0] = Vector4f(view.viewFrustum.planes[0].normal, view.viewFrustum.planes[0].distance);
            perFrameShaderParameters.viewFrustum[1] = Vector4f(view.viewFrustum.planes[1].normal, view.viewFrustum.planes[1].distance);
            perFrameShaderParameters.viewFrustum[2] = Vector4f(view.viewFrustum.planes[2].normal, view.viewFrustum.planes[2].distance);
            perFrameShaderParameters.viewFrustum[3] = Vector4f(view.viewFrustum.planes[3].normal, view.viewFrustum.planes[3].distance);
            perFrameShaderParameters.viewFrustum[4] = Vector4f(view.viewFrustum.planes[4].normal, view.viewFrustum.planes[4].distance);
            perFrameShaderParameters.viewFrustum[5] = Vector4f(view.viewFrustum.planes[5].normal, view.viewFrustum.planes[5].distance);

            perFrameShaderParameters.cameraJitterOffset = cameraJitterOffset;
            perFrameShaderParameters.previousCameraJitterOffset = historyFrame.cameraJitterOffset;
            perFrameShaderParameters.motionVectorJitterCancellation = (historyFrame.cameraJitterOffset - cameraJitterOffset) * Vector2f(1.0f / renderResolution.width, 1.0f / renderResolution.height);

            perFrameShaderParameters.previousCameraPosition = historyFrame.cameraPosition;

            perFrameShaderParameters.viewSpaceDepthToNDCSpaceDepthTransform = view.transformations.viewSpaceDepthToNDCSpaceDepthTransform;

            perFrameShaderParameters.worldToViewMatrix = view.transformations.worldToViewMatrix;
            perFrameShaderParameters.viewToWorldMatrix = view.transformations.viewToWorldMatrix;
            perFrameShaderParameters.viewToClipMatrix = view.transformations.viewToClipMatrix;
            perFrameShaderParameters.clipToViewMatrix = view.transformations.clipToViewMatrix;
            perFrameShaderParameters.worldToClipMatrix = view.transformations.worldToClipMatrix;
            perFrameShaderParameters.clipToWorldMatrix = view.transformations.clipToWorldMatrix;
            perFrameShaderParameters.nonJitteredWorldToClipMatrix = view.transformations.nonJitteredWorldToClipMatrix;
            perFrameShaderParameters.nonJitteredClipToWorldMatrix = view.transformations.nonJitteredClipToWorldMatrix;

            perFrameShaderParameters.previousWorldToViewMatrix = historyFrame.transformations.worldToViewMatrix;
            perFrameShaderParameters.previousViewToWorldMatrix = historyFrame.transformations.viewToWorldMatrix;
            perFrameShaderParameters.previousViewToClipMatrix = historyFrame.transformations.viewToClipMatrix;
            perFrameShaderParameters.previousClipToViewMatrix = historyFrame.transformations.clipToViewMatrix;
            perFrameShaderParameters.previousWorldToClipMatrix = historyFrame.transformations.worldToClipMatrix;
            perFrameShaderParameters.previousClipToWorldMatrix = historyFrame.transformations.clipToWorldMatrix;
            perFrameShaderParameters.previousNonJitteredWorldToClipMatrix = historyFrame.transformations.nonJitteredWorldToClipMatrix;

            // TODO: Precision loss
            reprojectionMatrix = perFrameShaderParameters.previousNonJitteredWorldToClipMatrix * perFrameShaderParameters.nonJitteredClipToWorldMatrix;
            inverseReprojectionMatrix = glm::inverse(reprojectionMatrix);

            perFrameShaderParameters.currentClipToPreviousClipMatrix = reprojectionMatrix;
            perFrameShaderParameters.previousClipToCurrentClipMatrix = inverseReprojectionMatrix;

            perFrameShaderParameters.materialTextureMipLodBias = materialTextureMipLodBias;

            perFrameShaderParameters.preExposure = preExposure;
            perFrameShaderParameters.inversePreExposure = 1.0f / preExposure;
            perFrameShaderParameters.preExposureCorrection = preExposure / historyFrame.preExposure;

            perFrameShaderParameters.motionVectorScale = Vector2f(float(renderResolution.width), float(renderResolution.height));

            // TODO: move this to other place?
            historyFrame.preExposure = preExposure;

            perFrameShaderParameters.indirectLightingMultiplier = rendererSettings.globalIlluminationSettings.indirectLightingIntensity * rendererSettings.globalIlluminationSettings.indirectLightingColor;

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

                    perFrameShaderParameters.atmosphericLightDirection = atmosphericLight->GetDirection();
                    perFrameShaderParameters.atmosphericLightDiskLuminance = atmosphericLightDiskLuminance;
                    perFrameShaderParameters.atmosphericLightDiskCosHalfApexAngle = cosHalfApexAngle;
                    perFrameShaderParameters.atmosphericLightOuterSpaceIlluminance = atmosphericLightIlluminance;
                }
                else
                {
                    perFrameShaderParameters.atmosphericLightDirection = DefaultLightDirection;
                    perFrameShaderParameters.atmosphericLightDiskLuminance = Vector3f(0.0f, 0.0f, 0.0f);
                    perFrameShaderParameters.atmosphericLightDiskCosHalfApexAngle = 1.0f;
                    perFrameShaderParameters.atmosphericLightOuterSpaceIlluminance = Vector3f(0.0f, 0.0f, 0.0f);
                }

                if (renderFeatures.enableSkyAtmosphereRendering)
                {
                    const SkyAtmosphereRenderObject& skyAtmosphere = *scene->GetActiveSkyAtmosphere();

                    SkyAtmosphereShaderParameters skyAtmosphereShaderParameters = {};
                    SetupSkyAtmosphereShaderParameters(skyAtmosphereShaderParameters, skyAtmosphere);

                    SkyAtmosphereViewRelatedParameters skyAtmosphereViewRelatedParameters =
                    {
                        .skyViewLutReferential = IdentityMatrix3x3f,
                    };
                    SetupSkyAtmosphereViewRelatedParameters(skyAtmosphereViewRelatedParameters, skyAtmosphere, view.cameraPosition, view.cameraForwardVector);

                    perFrameShaderParameters.skyAtmosphereTransmittanceLutSize = skyAtmosphereShaderParameters.transmittanceLutSize;
                    perFrameShaderParameters.skyAtmosphereMultipleScatteringLutSize = skyAtmosphereShaderParameters.multipleScatteringLutSize;
                    perFrameShaderParameters.skyAtmosphereSkyViewLutSize = skyAtmosphereShaderParameters.skyViewLutSize;
                    perFrameShaderParameters.skyAtmosphereAerialPerspectiveVolumeSize = skyAtmosphereShaderParameters.aerialPerspectiveVolumeSize;
                    perFrameShaderParameters.skyAtmosphereTransmittanceLutSampleCount = skyAtmosphereShaderParameters.transmittanceLutSampleCount;
                    perFrameShaderParameters.skyAtmosphereMultipleScatteringLutSampleCount = skyAtmosphereShaderParameters.multipleScatteringLutSampleCount;
                    perFrameShaderParameters.skyAtmosphereRayMarchingMinSampleCount = skyAtmosphereShaderParameters.rayMarchingMinSampleCount;
                    perFrameShaderParameters.skyAtmosphereRayMarchingMaxSampleCount = skyAtmosphereShaderParameters.rayMarchingMaxSampleCount;
                    perFrameShaderParameters.skyAtmosphereBottomRadiusInKilometers = skyAtmosphereShaderParameters.bottomRadius;
                    perFrameShaderParameters.skyAtmosphereTopRadiusInKilometers = skyAtmosphereShaderParameters.topRadius;
                    perFrameShaderParameters.skyAtmosphereGroundAlbedo = skyAtmosphereShaderParameters.groundAlbedo;
                    perFrameShaderParameters.skyAtmosphereRayleighScattering = skyAtmosphereShaderParameters.rayleighScattering;
                    perFrameShaderParameters.skyAtmosphereRayleighDensityExpScale = skyAtmosphereShaderParameters.rayleighDensityExpScale;
                    perFrameShaderParameters.skyAtmosphereMieScattering = skyAtmosphereShaderParameters.mieScattering;
                    perFrameShaderParameters.skyAtmosphereMieAbsorption = skyAtmosphereShaderParameters.mieAbsorption;
                    perFrameShaderParameters.skyAtmosphereMieExtinction = skyAtmosphereShaderParameters.mieExtinction;
                    perFrameShaderParameters.skyAtmosphereMiePhaseG = skyAtmosphereShaderParameters.miePhaseG;
                    perFrameShaderParameters.skyAtmosphereMieDensityExpScale = skyAtmosphereShaderParameters.mieDensityExpScale;
                    perFrameShaderParameters.skyAtmosphereAbsorptionDensity0LayerWidth = skyAtmosphereShaderParameters.absorptionDensity0LayerWidth;
                    perFrameShaderParameters.skyAtmosphereAbsorptionDensity0ConstantTerm = skyAtmosphereShaderParameters.absorptionDensity0ConstantTerm;
                    perFrameShaderParameters.skyAtmosphereAbsorptionDensity0LinearTerm = skyAtmosphereShaderParameters.absorptionDensity0LinearTerm;
                    perFrameShaderParameters.skyAtmosphereAbsorptionDensity1ConstantTerm = skyAtmosphereShaderParameters.absorptionDensity1ConstantTerm;
                    perFrameShaderParameters.skyAtmosphereAbsorptionDensity1LinearTerm = skyAtmosphereShaderParameters.absorptionDensity1LinearTerm;
                    perFrameShaderParameters.skyAtmosphereAbsorptionExtinction = skyAtmosphereShaderParameters.absorptionExtinction;
                    perFrameShaderParameters.skyAtmosphereSkyLuminanceFactor = skyAtmosphere.GetSkyLuminanceFactor();
                    perFrameShaderParameters.skyAtmosphereSkyViewLutReferential = skyAtmosphereViewRelatedParameters.skyViewLutReferential;
                }
            }

            // TODO: Move post processing settings form RasterizationRendererUniformVariables to other place
            {
                perFrameShaderParameters.motionBlurIntensity = finalPostProcessingSettings.motionBlurIntensity;
                perFrameShaderParameters.motionBlurMaxVelocityLengthInPixels = finalPostProcessingSettings.motionBlurMaxVelocityLength / 100.0f * std::max(targetResolution.width, targetResolution.height);

                perFrameShaderParameters.autoExposureExposureCompensation = finalPostProcessingSettings.autoExposureExposureCompensation;
                perFrameShaderParameters.autoExposureMinExposureValue = finalPostProcessingSettings.autoExposureMinExposureValue;
                perFrameShaderParameters.autoExposureMaxExposureValue = finalPostProcessingSettings.autoExposureMaxExposureValue;
                perFrameShaderParameters.autoExposureSpeedDarkToBright = finalPostProcessingSettings.autoExposureSpeedDarkToBright;
                perFrameShaderParameters.autoExposureSpeedBrightToDark = finalPostProcessingSettings.autoExposureSpeedBrightToDark;
                perFrameShaderParameters.autoExposureHistogramLowerPercentage = finalPostProcessingSettings.autoExposureHistogramLowerPercentage;
                perFrameShaderParameters.autoExposureHistogramHigherPercentage = finalPostProcessingSettings.autoExposureHistogramHigherPercentage / 100.0f;
                perFrameShaderParameters.autoExposureHistogramMinEV100 = finalPostProcessingSettings.autoExposureHistogramMinEV100;
                perFrameShaderParameters.autoExposureHistogramMaxEV100 = finalPostProcessingSettings.autoExposureHistogramMaxEV100;
                perFrameShaderParameters.autoExposureUseTargetExposure = (view.NeedToBeReset() || !renderFeatures.enableAutoExposure) ? 1.0f : 0.0f; // TODO: forceUseTargetExposure;
                perFrameShaderParameters.autoExposureMinimumLuminance = std::exp2(finalPostProcessingSettings.autoExposureHistogramMinEV100);

                perFrameShaderParameters.bloomIntensity = finalPostProcessingSettings.bloomIntensity;
                perFrameShaderParameters.bloomRadius = finalPostProcessingSettings.bloomRadius;

                perFrameShaderParameters.lensFlareIntensity = finalPostProcessingSettings.lensFlareIntensity;
                perFrameShaderParameters.lensFlareHaloIntensity = finalPostProcessingSettings.lensFlareHaloIntensity;
                perFrameShaderParameters.lensFlareHaloWidth = finalPostProcessingSettings.lensFlareHaloWidth;
                perFrameShaderParameters.lensFlareHaloChromaticAberrationOffset = finalPostProcessingSettings.lensFlareHaloChromaticAberrationOffset;

                perFrameShaderParameters.chromaticAberrationIntensity = finalPostProcessingSettings.chromaticAberrationIntensity;
                perFrameShaderParameters.chromaticAberrationOffset = finalPostProcessingSettings.chromaticAberrationOffset;

                perFrameShaderParameters.whiteBalance = finalPostProcessingSettings.whiteBalance;

                perFrameShaderParameters.vignetteIntensity = finalPostProcessingSettings.vignetteIntensity;

                //perFrameShaderParameters.toneMappingOperator = finalPostProcessingSettings.toneMappingOperator;

                //perFrameShaderParameters.colorCorrectionSaturation = finalPostProcessingSettings.colorCorrectionSaturation;
                //perFrameShaderParameters.colorCorrectionContrast = finalPostProcessingSettings.colorCorrectionContrast;
                //perFrameShaderParameters.colorCorrectionGamma = finalPostProcessingSettings.colorCorrectionGamma;
                //perFrameShaderParameters.colorCorrectionGain = finalPostProcessingSettings.colorCorrectionGain;
                //perFrameShaderParameters.colorCorrectionOffset = finalPostProcessingSettings.colorCorrectionOffset;
            }
        }
    }

#if 0
    bool hardwareRayTracingSupport = GRenderer->IsHardwareRayTracingEnabled();
    if (hardwareRayTracingSupport &&
        (IsSurfelGIEnabled() ||
            IsRayTracingShadowsEnabled() ||
            IsRayTracingReflectionsEnabled() ||
            IsRayTracingAmbientOcclusionEnabled()))
    {
        GRenderer->SetShouldUpdateRayTracingScene(true);
    }
#elif 0
    bool hardwareRayTracingSupport = GRenderer->IsHardwareRayTracingEnabled();
    if (hardwareRayTracingSupport)
    {
        GRenderer->SetShouldUpdateRayTracingScene(true);
    }
#endif

    RenderBackendTextureClearValue clearColor = RenderBackendTextureClearValue::Black;
    RenderBackendTextureClearValue clearDepth = RenderBackendTextureClearValue::CreateDepthValue(FAR_CLIPPING_PLANE_DEPTH_VALUE);
    RenderBackendTextureClearValue clearVisibilityBufferColor = RenderBackendTextureClearValue::CreateColorValueUnit4(0, 0, 0, 0);

    void RasterizationRenderer::Render(RenderGraph& renderGraph)
    {
        OPTICK_EVENT();

        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "RasterizationRenderer");

        // GatherRayTracingInstances();

        if (!perFrameConstantBuffers[currentPerFrameDataBufferIndex])
        {
            RenderBackendBufferDescription perFrameConstantUploadBufferDesc = RenderBackendBufferDescription::CreateUpload(sizeof(RasterizationRendererUniformVariables));
            perFrameConstantUploadBuffers[currentPerFrameDataBufferIndex] = renderBackend->CreateBuffer(&perFrameConstantUploadBufferDesc, nullptr, "PerFrameConstantUploadBuffer");
            RenderBackendBufferDescription perFrameConstantBufferDesc = RenderBackendBufferDescription::CreateStructured(sizeof(RasterizationRendererUniformVariables), 1);
            perFrameConstantBuffers[currentPerFrameDataBufferIndex] = renderBackend->CreateBuffer(&perFrameConstantBufferDesc, nullptr, "PerFrameConstantBuffer");
        }
        renderBackend->UpdateBuffer(perFrameConstantUploadBuffers[currentPerFrameDataBufferIndex], 0, &perFrameShaderParameters, sizeof(RasterizationRendererUniformVariables));

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
                    //RenderBackendBarrier transitionBefore = RenderBackendBarrier(perFrameConstantBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst);
                    //commandList.Barriers(&transitionBefore, 1);

                    commandList.CopyBuffer(
                        perFrameConstantUploadBuffer,
                        0,
                        perFrameConstantBuffer,
                        0,
                        sizeof(RasterizationRendererUniformVariables));

                    RenderBackendBarrier transitionAfter = RenderBackendBarrier(perFrameConstantBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::ShaderResource);
                    commandList.Barriers(&transitionAfter, 1);
                };
            });

        const SceneView& view = *sceneView;
        const RenderSettings& renderSettings = view.GetRenderSettings();
        RenderScene* renderScene = view.GetRenderScene();

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Create<RasterizationRendererIntermediateResources>();
        //RasterizationRendererDebugViewModeTextures& debugViewModeTextures = renderGraph.blackboard.Create<RasterizationRendererDebugViewModeTextures>();

        RenderGraphTextureDescription vbuffer0Desc = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R32Uint,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget,
            clearVisibilityBufferColor);
        intermediateResources.vbuffer0 = renderGraph.CreateTexture(vbuffer0Desc, "VBuffer0");

        RenderGraphTextureDescription vbuffer1Desc = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R32G32B32A32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget,
            clearVisibilityBufferColor);
        intermediateResources.vbuffer1 = renderGraph.CreateTexture(vbuffer1Desc, "VBuffer1");

        RenderGraphTextureDescription gbuffer0Desc = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R10G10B10A2Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            clearColor);
        intermediateResources.gbuffer0 = renderGraph.CreateTexture(gbuffer0Desc, "GBuffer0");

        RenderGraphTextureDescription gbuffer1Desc = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R8G8B8A8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            clearColor);
        intermediateResources.gbuffer1 = renderGraph.CreateTexture(gbuffer1Desc, "GBuffer1");

        RenderGraphTextureDescription gbuffer2Desc = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R8G8B8A8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            clearColor);
        intermediateResources.gbuffer2 = renderGraph.CreateTexture(gbuffer2Desc, "GBuffer2");

        RenderGraphTextureDescription colorTextureDescription = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget,
            clearColor,
            1,
            1,
            RenderBackendResourceState::UnorderedAccess);
        intermediateResources.colorTexture = renderGraph.CreateTexture(colorTextureDescription, "ColorTexture");

        RenderGraphTextureDescription sceneDepthTextureDescription = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::D32FloatS8Uint,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::DepthStencil,
            clearDepth,
            1,
            1,
            RenderBackendResourceState::DepthStencil);
        intermediateResources.depthTexture = renderGraph.CreateTexture(sceneDepthTextureDescription, "DepthTexture");

        RenderGraphTextureDescription motionVectorTextureDescription = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R16G16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        intermediateResources.motionVectorTexture = renderGraph.CreateTexture(motionVectorTextureDescription, "MotionVectorTexture");

        if (renderFeatures.enableSkyAtmosphereRendering)
        {
            RenderSkyAtmosphereLUTs(renderGraph, view);
        }

        DispatchLocalLightCulling(renderGraph, view);

        DispatchVisibilityCulling(renderGraph, view);

        RenderVisibilityBuffer(renderGraph, view);

        RenderGBuffer(renderGraph, view);

        RenderMotionVectors(renderGraph, view);

        DispatchDepthPyramidGeneration(renderGraph, view);

        if (rendererSettings.shadowsTechnique == RasterizationRendererShadowsTechnique::ShadowMaps)
        {
            RenderShadowMapDepth(renderGraph, view);
        }
        else if (rendererSettings.shadowsTechnique == RasterizationRendererShadowsTechnique::VirtualShadowMaps)
        {
            RenderVirtualShadowMapDepth(renderGraph, view);
        }

        CaptureEnvironmentMap(renderGraph, view);

        if (renderBackend->GetType() == RenderBackendType::Vulkan)
        {
            renderGraph.AddPass(
            std::format("ClearSceneTextures"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneColorTexture = intermediateResources.colorTexture = builder.WriteTexture(intermediateResources.colorTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendTextureClearValue clearValue = RenderBackendTextureClearValue::Black;

                    RenderBackendTextureUAVDesc sceneColorTextureUAV = RenderBackendTextureUAVDesc::Create(resourceRegistry.GetRenderBackendTextureHandle(sceneColorTexture), 0);
                    commandList.ClearTextureUAV(sceneColorTextureUAV, clearValue);
                };
            });
        }
        else
        {
            renderGraph.AddPass(
            std::format("ClearSceneTextures"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneColorTexture = intermediateResources.colorTexture = builder.WriteTexture(intermediateResources.colorTexture, RenderBackendResourceState::RenderTarget);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendTextureClearValue clearValue = RenderBackendTextureClearValue::Black;
                    clearValue.test = true;

                    RenderBackendTextureUAVDesc sceneColorTextureUAV = RenderBackendTextureUAVDesc::Create(resourceRegistry.GetRenderBackendTextureHandle(sceneColorTexture), 0);
                    commandList.ClearTextureUAV(sceneColorTextureUAV, clearValue);
                };
            });
        }

        if (renderFeatures.enableScreenSpaceAmbientOcclusion)
        {
            intermediateResources.ambientOcclusionTexture = RenderScreenSpaceAmbientOcclusion(renderGraph, view);
        }
        else if (renderFeatures.enableRayTracingAmbientOcclusion)
        {
            intermediateResources.ambientOcclusionTexture = RenderRayTracingAmbientOcclusion(renderGraph, view);
        }
        else
        {
            intermediateResources.ambientOcclusionTexture = defaultResources->ImportWhiteDummyTexture2D(renderGraph);
        }

        if (rendererSettings.debugVisualizationMode == RasterizationRendererDebugVisualizationMode::AmbientOcclusion)
        {
            debugVisualizationCallback = std::bind(&RasterizationRenderer::DispatchAmbientOcclusionDebugVisualization, this, std::placeholders::_1, std::placeholders::_2);
        }

        RenderScreenSpaceIndirectDiffuse(renderGraph, view);

        AddIndirectLightingDiffusePass(renderGraph, view);

        if (renderFeatures.enableScreenSpaceReflections)
        {
            intermediateResources.indirectSpecularTexture = RenderScreenSpaceReflections(renderGraph, view);
        }
        else
        {
            intermediateResources.indirectSpecularTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        }

        // else if (settings.reflectionsTechnique == ReflectionsTechnique::RayTracingReflections)
        // {
        //     //RenderRayTracingReflections();
        // }
        // else
        // {
        //     renderGraph.AddPass("ClearReflectionTexture", RenderGraphPassFlags::Compute,
        //         [&](RenderGraphBuilder& builder)
        //         {
        //             reflectionsTexture = builder.WriteTexture(reflectionsTexture, RenderBackendResourceState::UnorderedAccess);
        //
        //
        //             {
        //                 commandList.ClearTextureUAV(
        //                     resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndexreflectionsTexture), 0),
        //                     RenderBackendTextureClearValue::Black);
        //             };
        //         });
        // }

        AddIndirectLightingSpecularPass(renderGraph, view);

        // AddSurfleGIPasses(renderGraph, view);

        RenderGraphTextureDescription screenSpaceShadowMaskTextureDesc = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R8G8B8A8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture");

        //if (view.visualizationMode == SceneViewVisualizationMode::ShadowMask)
        //{
        //    debugViewModeTextures.screenSpaceShadowMaskTextureDesc = screenSpaceShadowMaskTextureDesc;
        //    debugViewModeTextures.screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture (Copy)");
        //}

        RenderGraphTextureDescription rayDistanceTextureDesc = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle rayDistanceTexture = renderGraph.CreateTexture(rayDistanceTextureDesc, "RayTracingShadowsRayDistanceTexture");

        if (rendererSettings.shadowsTechnique == RasterizationRendererShadowsTechnique::ShadowMaps)
        {
            DispatchShadowMapProjection(renderGraph, view);

            const LightRenderObject* light = renderScene->GetAtmosphericLight();
            if (light && light->enableScreenSpaceShadows)
            {
                DispatchScreenSpaceShadows(renderGraph, view, *light);
            }
        }
        else if (rendererSettings.shadowsTechnique == RasterizationRendererShadowsTechnique::VirtualShadowMaps)
        {
            DispatchVirtualShadowMapProjection(renderGraph, view);
        }
        else if (rendererSettings.shadowsTechnique == RasterizationRendererShadowsTechnique::RayTracingShadows)
        {
            const LightRenderObject* light = renderScene->GetAtmosphericLight();
            if (light)
            {
                DispatchRayTracingShadows(renderGraph, view, *light, screenSpaceShadowMaskTexture, rayDistanceTexture);

                intermediateResources.shadowMaskTexture = screenSpaceShadowMaskTexture;
            }
        }
        else
        {
            intermediateResources.shadowMaskTexture = defaultResources->ImportWhiteDummyTexture2D(renderGraph);
        }

        RenderGraphTextureHandle localLightShadowMapAtlas = RenderLocalLightShadows(renderGraph, view);

        AddDirectLightingPass(renderGraph, view, localLightShadowMapAtlas);

        if (renderFeatures.enableSubsurfaceScattering)
        {
            RenderSubsurfaceScattering(renderGraph, view);
        }

        if (renderFeatures.enableSkyAtmosphereRendering)
        {
            RenderSkyAtmosphere(renderGraph, view);
        }

        if (renderFeatures.enableVolumetricFog)
        {
            RenderVolumetricFog(renderGraph, view);
        }

        //RenderLocalFogVolumes(renderGraph, view);

        if (renderFeatures.enableScreenSpaceLightShafts)
        {
            RenderScreenSpaceLightShafts(renderGraph, view);
        }

        // @todo Refactor this
        intermediateResources.colorTexture = AddDebugDrawPass(renderGraph, view, intermediateResources.colorTexture, intermediateResources.depthTexture);

        ExecutePostProcessingPipeline(renderGraph, view);

        historyFrame.cameraJitterOffset = cameraJitterOffset;
        historyFrame.cameraPosition = view.GetCameraPosition();
        historyFrame.preExposure = preExposure;
        historyFrame.transformations = view.GetCameraTransformations();

        currentPerFrameDataBufferIndex = (currentPerFrameDataBufferIndex + 1) % MaxNumFramesInFlight;
    }
}