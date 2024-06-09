#include "RealTimeRenderer.h"
#include "RealTimeRendererPrivate.h"
#include "PerFrameShaderParameters.h"
#include "SkyAtmosphereRendering.h"

#include "FidelityFXSuperResolution2Module.h"

#include <optick.h>

namespace Horizon
{
#if 0
    RealTimeRenderer::RealTimeRenderer(RenderSystem* renderSystem)
        : renderBackend(backend)
        , shaderLibrary(shaderLibrary)
        , renderEngine(renderEngine)
    {
        RenderBackendBufferDesc perFrameDataBufferDesc = RenderBackendBufferDesc::CreateByteAddress(sizeof(PerFrameShaderParameters));
        for (uint32 index = 0; index < MaxNumFramesInFlight; index++)
        {
            perFrameDataBuffers[index] = renderBackend->CreateBuffer(&perFrameDataBufferDesc, nullptr, "PerFrameDataBuffer");
        }

        RenderBackendBufferDesc autoExposureBufferDesc = RenderBackendBufferDesc::CreateReadback(sizeof(AutoExposureData));
        autoExposureBufferHistory = resourcePool->AllocateBuffer(autoExposureBufferDesc, "AutoExposureBuffer");

        for (uint32 index = 0; index < NumAutoExposureReadbackBuffers; index++)
        {
            autoExposureReadbackBuffers[index] = resourcePool->AllocateBuffer(autoExposureBufferDesc, "AutoExposureReadBackBuffer");
        }

        //RenderBackendBufferDesc surfelGIInfoBufferDesc = RenderBackendBufferDesc::CreateByteAddress(SurfelGIInfoBufferSize);
        //surfelGIInfoBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGIInfoBufferDesc, nullptr, "SurfelGIInfoBuffer");

        //RenderBackendBufferDesc surfelGIArgumentBufferDesc = RenderBackendBufferDesc::CreateIndirectArguments(sizeof(uint32) * 12);
        //surfelGIArgumentBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGIArgumentBufferDesc, nullptr, "SurfelGIArgumentBuffer");

        //RenderBackendBufferDesc surfelGISurfelIndirectionBufferDesc = RenderBackendBufferDesc::CreateByteAddress(SurfelGIMaxSurfelCount * sizeof(uint32));
        //surfelGIAliveSurfelIndirectionBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGISurfelIndirectionBufferDesc, nullptr, "SurfelGIAliveSurfelIndirectionBuffer");
        //surfelGIFreeSurfelBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGISurfelIndirectionBufferDesc, nullptr, "SurfelGIFreeSurfelBuffer");

        //RenderBackendBufferDesc surfelGISurfelHotDataBufferDecs = RenderBackendBufferDesc::CreateByteAddress(SurfelGIMaxSurfelCount * sizeof(SurfelHotData));
        //surfelGISurfelHotDataBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGISurfelHotDataBufferDecs, nullptr, "SurfelGISurfelHotDataBuffer");

        //RenderBackendBufferDesc surfelGICellHeaderBufferDesc = RenderBackendBufferDesc::CreateByteAddress(SurfelGIUniformGridCellCount * sizeof(uint32) * 2);
        //surfelGICellHeaderBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGICellHeaderBufferDesc, nullptr, "SurfelGICellHeaderBuffer");

        //RenderBackendBufferDesc surfelGICellDataBufferDesc = RenderBackendBufferDesc::CreateByteAddress(SurfelGIMaxSurfelCount * sizeof(uint32));
        //surfelGICellDataBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGICellDataBufferDesc, nullptr, "SurfelGICellDataBuffer");

        //RenderBackendBufferDesc sceneViewShaderParametersBufferDesc = RenderBackendBufferDesc::CreateByteAddress(sizeof(RealTimeRendererSceneViewShaderParameters));
        //sceneViewShaderParametersBuffer = renderBackend->CreateBuffer(deviceMask, &sceneViewShaderParametersBufferDesc, nullptr, "SceneViewShaderParametersBuffer");

        //RenderBackendBufferDesc sceneViewShaderParametersUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(sizeof(RealTimeRendererSceneViewShaderParameters));
        //sceneViewShaderParametersUploadBuffer = renderBackend->CreateBuffer(deviceMask, &sceneViewShaderParametersUploadBufferDesc, nullptr, "SceneViewShaderParametersUploadBuffer");

        //testTexture = LoadTextureFromFile(renderBackend, "../../../Assets/PurkinjeShift.png", false);

        //RenderBackendBufferDesc ssrRayAllocationBufferDesc = RenderBackendBufferDesc::CreateIndirectArguments(sizeof(uint32), 12);
        //ssrRayAllocationBuffer = renderBackend->CreateBuffer(deviceMask, &ssrRayAllocationBufferDesc, nullptr, "SSRRayAllocationBuffer");

        //defaultBloomKernelTexture = LoadTextureFromHDRFile(renderBackend, "../../../Assets/HDRIs/DefaultBloomKernel.hdr", &defaultBloomKernelTextureDesc);

        //localExposureTestTexture = LoadTextureFromHDRFile(renderBackend, "../../../Assets/HDRIs/veranda_2k.hdr", &localExposureTestTextureDesc);

        //lensDirtTexture = LoadTextureFromFile(GRenderBackend, "../../../Assets/Textures/LensDirtTexture.png", false);
        //lensFlaresGlareLUTTexture = LoadTextureFromFile(GRenderBackend, "../../../Assets/Textures/LensFlaresGlareLUT.png", false);
        //lensFlaresGradiantLUTTexture = LoadTextureFromFile(GRenderBackend, "../../../Assets/Textures/LensFlaresGradiantLUT.png", false);

        //blueNoiseTexture = LoadTextureFromFile(GRenderBackend, "../../../Assets/Textures/BlueNoise.png", false);
        //{
            //uint8_t blueNoise[128][128][4] = {};
            //for (int x = 0; x < 128; ++x)
            //{
            //    for (int y = 0; y < 128; ++y)
            //    {
            //        float const f0 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 0);
            //        float const f1 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 1);
            //        float const f2 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 2);
            //        float const f3 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 3);
            //
            //        blueNoise[x][y][0] = static_cast<uint8_t>(f0 * 0xFF);
            //        blueNoise[x][y][1] = static_cast<uint8_t>(f1 * 0xFF);
            //        blueNoise[x][y][2] = static_cast<uint8_t>(f2 * 0xFF);
            //        blueNoise[x][y][3] = static_cast<uint8_t>(f3 * 0xFF);
            //    }
            //}
            //
            //RenderGraphTextureDesc blueNoiseTextureDesc = RenderGraphTextureDesc::Create2D(
            //    128,
            //    128,
            //    RenderBackendTextureFormat::RGBA8Unorm,
            //    RenderBackendTextureCreateFlags::ShaderResource);
            //blueNoiseTexture = renderBackend->CreateTexture(deviceMask, &blueNoiseTextureDesc, blueNoise, "BlueNoiseTexture");
        //}
    }
#endif
    RealTimeRenderer::RealTimeRenderer(
        RenderBackend* renderBackend,
        RenderGraphResourcePool* resourcePool,
        ShaderLibrary* shaderLibrary,
        RendererDefaultResources* defaultResources)
        : renderBackend(renderBackend)
        , resourcePool(resourcePool)
        , shaderLibrary(shaderLibrary)
        , defaultResources(defaultResources)
        , temporalSuperSamplingInterface(nullptr)
    {
        RenderBackendBufferDesc perFrameDataBufferDesc = RenderBackendBufferDesc::CreateByteAddress(sizeof(PerFrameShaderParameters), true);
        for (uint32 index = 0; index < MaxNumFramesInFlight; index++)
        {
            perFrameDataBuffers[index] = renderBackend->CreateBuffer(&perFrameDataBufferDesc, nullptr, "PerFrameDataBuffer");
        }

        RenderBackendBufferDesc autoExposureReadbackBufferDesc = RenderBackendBufferDesc::CreateReadback(sizeof(AutoExposureData));
        for (uint32 index = 0; index < NumAutoExposureReadbackBuffers; index++)
        {
            autoExposureReadbackBuffers[index] = resourcePool->AllocateBuffer(autoExposureReadbackBufferDesc, "AutoExposureReadBackBuffer");
        }

        ResetHistoryFrame();
    }

    RealTimeRenderer::~RealTimeRenderer()
    {

    }

    RenderBackendBufferHandle RealTimeRenderer::GetCurrentPerFrameDataBuffer() const
    {
        return perFrameDataBuffers[currentPerFrameDataBufferIndex];
    }

    void RealTimeRenderer::UpdateAutoExposureDataFromReadbackBuffer()
    {
        RenderGraphPersistentBuffer* autoExposureReadbackBuffer = autoExposureReadbackBuffers[currentAutoExposureReadbackBufferIndex];
        if (autoExposureReadbackBuffer != nullptr)
        {
            void* data = nullptr;
            renderBackend->MapBuffer(autoExposureReadbackBuffer->GetHandle(), &data);
            if (data != nullptr)
            {
                autoExposureData.adaptedExposure = static_cast<float*>(data)[0];
                autoExposureData.targetExposure = static_cast<float*>(data)[1];
                autoExposureData.exposureCompensation = static_cast<float*>(data)[2];
                autoExposureData.averageSceneLuminance = static_cast<float*>(data)[3];
                renderBackend->UnmapBuffer(autoExposureReadbackBuffer->GetHandle());
            }
        }
    }

    void RealTimeRenderer::ResetHistoryFrame()
    {
        historyFrame.cameraPosition = Vector3(0.0f, 0.0f, 0.0f);
        historyFrame.cameraJitterOffset = Vector2(0.0f, 0.0f);
        historyFrame.transformations.Reset();
        historyFrame.preExposure = 1.0f;

        RenderBackendBufferDesc autoExposureBufferDesc = RenderBackendBufferDesc::Create(sizeof(AutoExposureData), 1, RenderBackendBufferCreateFlags::ShaderResource | RenderBackendBufferCreateFlags::UnorderedAccess);
        historyFrame.autoExposureBuffer = resourcePool->AllocateBuffer(autoExposureBufferDesc, "AutoExposureBuffer");
    }

    bool RealTimeRenderer::IsBloomEnabled() const
    {
        return IsGaussianBloomEnabled() || IsConvolutionBloomEnabled();
    }

    void RealTimeRenderer::OnRenderBegin(SceneView* v)
    {
        sceneView = v;
        // Super Resolution
        //isDLSSEnabled = false;
        //isFSR2Enabled = false;
        //isSuperResolutionEnabled = false;
        //switch (view.renderSettings.superResolutionTechnique)
        //{
        //case SuperResolutionTechnique::FSR2:
        //    {
        //        isFSR2Enabled = true;
        //        isSuperResolutionEnabled = true;
        //    } break;
        //case SuperResolutionTechnique::DLSSSuperResolution:
        //    {
        //        isDLSSEnabled = true;
        //        isSuperResolutionEnabled = true;
        //    } break;
        //default: break;
        //}

        //// Antialiasing
        //isDLAAEnabled = false;
        //isTemporalAAEnabled = false;
        //if (!IsSuperResolutionEnabled())
        //{
        //    switch (view.renderSettings.antialiasingTechnique)
        //    {
        //    case AntialiasingTechnique::TemporalAA:
        //        {
        //            isTemporalAAEnabled = true;
        //        } break;
        //    case AntialiasingTechnique::DLAA:
        //        {
        //            isDLAAEnabled = true;
        //        } break;
        //    default: break;
        //    }
        //}

        SceneView& view = *sceneView;

        renderResolutionPercentage = 1.0f;

        if (temporalSuperSamplingInterface == nullptr)
        {
            temporalSuperSamplingInterface = new FidelityFXSuperResolution2(renderBackend);
        }

        renderResolution = Extent2D(view.targetWidth, view.targetHeight);
        targetResolution = Extent2D(view.targetWidth, view.targetHeight);
        displayResolution = Extent2D(view.targetWidth, view.targetHeight);

        cameraJitterOffset = Vector2(0.0f, 0.0f);

        if (temporalSuperSamplingInterface != nullptr)
        {
            TemporalSuperSamplingOptions tssOptions = {};
            tssOptions.targetWidth = targetResolution.width;
            tssOptions.targetHeight = targetResolution.height;
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

        Vector2 previousCameraJitterOffset = perFrameShaderParameters.cameraJitterOffset;
        if (view.frameIndex == 0)
        {
            previousCameraJitterOffset = cameraJitterOffset;
        }

        Vector3 previousCameraPosition = perFrameShaderParameters.cameraPosition;

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

        //isSurfelGIEnabled = false;
        //isRayTracingShadowsEnabled = settings.shadowsTechnique == ShadowsTechnique::RayTracingShadows;
        //isRayTracingReflectionsEnabled = settings.reflectionsTechnique == ReflectionsTechnique::RayTracingReflections;
        //isRayTracingAmbientOcclusionEnabled = settings.ambientOcclusionTechnique == AmbientOcclusionTechnique::RayTracingAmbientOcclusion;

        finalPostProcessingSettings = view.renderSettings.postProcessingSettings;

        const RenderScene* scene = view.GetRenderScene();

        features.enableSuperResolution = temporalSuperSamplingInterface != nullptr;

        features.enableSkyAtmosphereRendering =
            scene != nullptr &&
            scene->HasAtmosphericLight() &&
            scene->HasActiveSkyAtmosphere();

        features.enableScreenSpaceAmbientOcclusion = false;
        features.enableScreenSpaceLightShafts = false;

        features.enableDepthOfField = false;// finalPostProcessingSettings.depthOfFieldScale > 0.0f;
        features.enableMotionBlur = false;

        features.enableAutoExposure = (finalPostProcessingSettings.exposureMethod == ExposureMethod::AutoExposure);

        features.enableGaussianBloom = finalPostProcessingSettings.bloomIntensity > 0.0f;

        features.enableConvolutionBloom = false;

        if (!IsAutoExposureEnabled())
        {
            finalPostProcessingSettings.autoExposureMinExposureValue = finalPostProcessingSettings.fixedExposureValue;
            finalPostProcessingSettings.autoExposureMinExposureValue = finalPostProcessingSettings.fixedExposureValue;
        }

        UpdatePerFrameDataBuffer();

        if (temporalSuperSamplingInterface != nullptr)
        {
            TemporalSuperSamplingConstants tssConstants = {};
            tssConstants.reset = false;
            tssConstants.sharpness = 0.0f;
            tssConstants.deltaTime = view.deltaTimeInSeconds * 1000.0f;
            tssConstants.preExposure = preExposure;
            tssConstants.renderWidth = renderResolution.width;
            tssConstants.renderHeight = renderResolution.height;
            tssConstants.jitterOffsetX = cameraJitterOffset.x;
            tssConstants.jitterOffsetY = cameraJitterOffset.y;
            tssConstants.motionVectorScaleX = float(renderResolution.width);
            tssConstants.motionVectorScaleY = float(renderResolution.height);
            tssConstants.cameraNearClippingPlane = view.nearClippingPlane;
            tssConstants.cameraFarClippingPlane = view.farClippingPlane;
            tssConstants.cameraFieldOfView = Math::DegreesToRadians(view.fieldOfView);

            temporalSuperSamplingInterface->SetConstants(tssConstants);
        }
    }

    void RealTimeRenderer::UpdatePerFrameDataBuffer()
    {
        const SceneView& view = *sceneView;
        const RenderSettings& renderSettings = view.renderSettings;
        const RenderScene* scene = view.scene;

        // Setup PerFrameShaderParameters
        PerFrameShaderParameters perFrameShaderParameters = {};
        {
            perFrameShaderParameters.frameIndex = view.frameIndex;

            perFrameShaderParameters.deltaTimeInSeconds = view.deltaTimeInSeconds;

            perFrameShaderParameters.renderWidth = renderResolution.width;
            perFrameShaderParameters.renderHeight = renderResolution.height;
            perFrameShaderParameters.targetWidth = targetResolution.width;
            perFrameShaderParameters.targetHeight = targetResolution.height;
            perFrameShaderParameters.displayWidth = displayResolution.width;
            perFrameShaderParameters.displayHeight = displayResolution.height;

            perFrameShaderParameters.renderResolution = Vector4(1.0f * renderResolution.width, 1.0f * renderResolution.height, 1.0f / renderResolution.width, 1.0f / renderResolution.height);
            perFrameShaderParameters.targetResolution = Vector4(1.0f * targetResolution.width, 1.0f * targetResolution.height, 1.0f / targetResolution.width, 1.0f / targetResolution.height);
            perFrameShaderParameters.displayResolution = Vector4(1.0f * displayResolution.width, 1.0f * displayResolution.height, 1.0f / displayResolution.width, 1.0f / displayResolution.height);

            perFrameShaderParameters.cameraPosition = view.cameraPosition;
            perFrameShaderParameters.cameraUpVector = view.cameraUpVector;
            perFrameShaderParameters.cameraRightVector = view.cameraRightVector;
            perFrameShaderParameters.cameraForwardVector = view.cameraForwardVector;
            perFrameShaderParameters.halfFovInRadians = Math::DegreesToRadians(view.fieldOfView) * 0.5f;
            perFrameShaderParameters.aspectRatio = view.aspectRatio;
            perFrameShaderParameters.nearClippingPlane = view.nearClippingPlane;
            perFrameShaderParameters.farClippingPlane = view.farClippingPlane;

            perFrameShaderParameters.cameraJitterOffset = cameraJitterOffset;

            perFrameShaderParameters.previousCameraPosition = historyFrame.cameraPosition;
            perFrameShaderParameters.previousCameraJitterOffset = historyFrame.cameraJitterOffset;

            perFrameShaderParameters.worldToViewMatrix = view.transformations.worldToViewMatrix;
            perFrameShaderParameters.viewToWorldMatrix = view.transformations.viewToWorldMatrix;
            perFrameShaderParameters.viewToClipMatrix = view.transformations.viewToClipMatrix;
            perFrameShaderParameters.clipToViewMatrix = view.transformations.clipToViewMatrix;
            perFrameShaderParameters.worldToClipMatrix = view.transformations.worldToClipMatrix;
            perFrameShaderParameters.clipToWorldMatrix = view.transformations.clipToWorldMatrix;
            perFrameShaderParameters.nonJitteredWorldToClipMatrix = view.transformations.nonJitteredViewToClipMatrix;

            perFrameShaderParameters.previousWorldToViewMatrix = historyFrame.transformations.worldToViewMatrix;
            perFrameShaderParameters.previousViewToWorldMatrix = historyFrame.transformations.viewToWorldMatrix;
            perFrameShaderParameters.previousViewToClipMatrix = historyFrame.transformations.viewToClipMatrix;
            perFrameShaderParameters.previousClipToViewMatrix = historyFrame.transformations.clipToViewMatrix;
            perFrameShaderParameters.previousWorldToClipMatrix = historyFrame.transformations.worldToClipMatrix;
            perFrameShaderParameters.previousClipToWorldMatrix = historyFrame.transformations.clipToWorldMatrix;
            perFrameShaderParameters.previousNonJitteredWorldToClipMatrix = historyFrame.transformations.nonJitteredViewToClipMatrix;

            perFrameShaderParameters.materialTextureMipLodBias = materialTextureMipLodBias;

            UpdateAutoExposureDataFromReadbackBuffer();

            preExposure = autoExposureData.adaptedExposure;

            perFrameShaderParameters.preExposure = preExposure;
            perFrameShaderParameters.oneOverPreExposure = 1.0f / preExposure;
            perFrameShaderParameters.preExposureCorrection = preExposure / historyFrame.preExposure;

            // TODO: move this to other place?
            historyFrame.preExposure = preExposure;

            perFrameShaderParameters.indirectLightingMultiplier = renderSettings.globalIlluminationSettings.indirectLightingIntensity * renderSettings.globalIlluminationSettings.indirectLightingColor;

            if (scene != nullptr)
            {
                if (scene->HasAtmosphericLight())
                {
                    const LightRenderObject* atmosphericLight = scene->GetAtmosphericLight();
                    const float halfApexAngle = atmosphericLight->GetHalfApexAngleInRadians();
                    const float cosHalfApexAngle = std::cos(halfApexAngle);
                    const float solidAngle = 2.0f * M_PI * (1.0f - cosHalfApexAngle); // https://en.wikipedia.org/wiki/Solid_angle

                    const Vector3 atmosphericLightIlluminance = atmosphericLight->GetPhysicalLightColor();
                    const Vector3 atmosphericLightDiskLuminance = atmosphericLight->GetAtmosphericLightDiskColorFactor() * atmosphericLightIlluminance / solidAngle; // approximation

                    perFrameShaderParameters.atmosphericLightDirection = atmosphericLight->GetDirection();
                    perFrameShaderParameters.atmosphericLightDiskLuminance = atmosphericLightDiskLuminance;
                    perFrameShaderParameters.atmosphericLightDiskCosHalfApexAngle = cosHalfApexAngle;
                    perFrameShaderParameters.atmosphericLightOuterSpaceIlluminance = atmosphericLightIlluminance;
                }
                else
                {
                    perFrameShaderParameters.atmosphericLightDirection = DefaultLightDirection;
                    perFrameShaderParameters.atmosphericLightDiskLuminance = Vector3(0.0f, 0.0f, 0.0f);
                    perFrameShaderParameters.atmosphericLightDiskCosHalfApexAngle = 1.0f;
                    perFrameShaderParameters.atmosphericLightOuterSpaceIlluminance = Vector3(0.0f, 0.0f, 0.0f);
                }

                if (IsSkyAtmosphereRenderingEnabled())
                {
                    const SkyAtmosphereRenderObject& skyAtmosphere = *scene->GetActiveSkyAtmosphere();

                    SkyAtmosphereShaderParameters skyAtmosphereShaderParameters = {};
                    SetupSkyAtmosphereShaderParameters(skyAtmosphereShaderParameters, skyAtmosphere);

                    SkyAtmosphereViewRelatedParameters skyAtmosphereViewRelatedParameters =
                    {
                        .skyViewLutReferential = IdentityMatrix3x3,
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

            // TODO: Move post processing settings form PerFrameShaderParameters to other place
            {
                perFrameShaderParameters.autoExposureExposureCompensation = finalPostProcessingSettings.autoExposureExposureCompensation;
                perFrameShaderParameters.autoExposureMinExposureValue = finalPostProcessingSettings.autoExposureMinExposureValue;
                perFrameShaderParameters.autoExposureMaxExposureValue = finalPostProcessingSettings.autoExposureMaxExposureValue;
                perFrameShaderParameters.autoExposureSpeedDarkToBright = finalPostProcessingSettings.autoExposureSpeedDarkToBright;
                perFrameShaderParameters.autoExposureSpeedBrightToDark = finalPostProcessingSettings.autoExposureSpeedBrightToDark;
                perFrameShaderParameters.autoExposureHistogramLowerPercentage = finalPostProcessingSettings.autoExposureHistogramLowerPercentage;
                perFrameShaderParameters.autoExposureHistogramHigherPercentage = finalPostProcessingSettings.autoExposureHistogramHigherPercentage;
                perFrameShaderParameters.autoExposureHistogramMinEV100 = finalPostProcessingSettings.autoExposureHistogramMinEV100;
                perFrameShaderParameters.autoExposureHistogramMaxEV100 = finalPostProcessingSettings.autoExposureHistogramMaxEV100;
                perFrameShaderParameters.autoExposureUseTargetExposure = (view.NeedToBeReset() || !IsAutoExposureEnabled()) ? 1 : 0; // TODO: forceUseTargetExposure;

                perFrameShaderParameters.bloomIntensity = finalPostProcessingSettings.bloomIntensity;
                perFrameShaderParameters.bloomRadius = finalPostProcessingSettings.bloomRadius;

                perFrameShaderParameters.chromaticAberrationIntensity = finalPostProcessingSettings.chromaticAberrationIntensity;
                perFrameShaderParameters.chromaticAberrationOffset = finalPostProcessingSettings.chromaticAberrationOffset;

                perFrameShaderParameters.whiteBalance = finalPostProcessingSettings.whiteBalance;

                //perFrameShaderParameters.toneMappingOperator = finalPostProcessingSettings.toneMappingOperator;

                //perFrameShaderParameters.colorCorrectionSaturation = finalPostProcessingSettings.colorCorrectionSaturation;
                //perFrameShaderParameters.colorCorrectionContrast = finalPostProcessingSettings.colorCorrectionContrast;
                //perFrameShaderParameters.colorCorrectionGamma = finalPostProcessingSettings.colorCorrectionGamma;
                //perFrameShaderParameters.colorCorrectionGain = finalPostProcessingSettings.colorCorrectionGain;
                //perFrameShaderParameters.colorCorrectionOffset = finalPostProcessingSettings.colorCorrectionOffset;
            }
        }

        RenderBackendBufferHandle perFrameDataBuffer = GetCurrentPerFrameDataBuffer();

        void* data = nullptr;
        renderBackend->MapBuffer(perFrameDataBuffer, &data);
        memcpy(data, &perFrameShaderParameters, sizeof(PerFrameShaderParameters));
        renderBackend->UnmapBuffer(perFrameDataBuffer);
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

    RenderGraphTextureHandle RealTimeRenderer::RenderUserInterface(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RenderGraphTextureDesc uiColorAndAlphaTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolution.width,
            targetResolution.height,
            RenderBackendTextureFormat::RGB10A2Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle uiColorAndAlphaTexture = renderGraph.CreateTexture(uiColorAndAlphaTextureDesc, "UIColorAndAlphaTexture");

        renderGraph.AddPass(
            std::format("UIColorAndAlpha (Graphics, {}x{})", targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Graphics, // | RenderGraphPassFlags::SkipRenderPass,
            [&](RenderGraphBuilder& builder)
            {
                uiColorAndAlphaTexture = builder.WriteTexture(uiColorAndAlphaTexture, RenderBackendResourceState::RenderTarget);

                builder.BindRenderTarget(0, uiColorAndAlphaTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    //renderEngine->DrawUI(commandList, registry.GetRenderBackendTextureHandle(uiColorAndAlphaTexture));
                };
            });

        return uiColorAndAlphaTexture;
    }

#define NearClipPlaneDepthValue 1.0f
#define FarClipPlaneDepthValue 0.0f

    RenderBackendTextureClearValue clearColor = RenderBackendTextureClearValue::CreateColorValueFloat4(0.0f, 0.0f, 0.0f, 0.0f);
    RenderBackendTextureClearValue clearDepth = RenderBackendTextureClearValue::CreateDepthValue(FarClipPlaneDepthValue);
    RenderBackendTextureClearValue clearVisibilityBufferColor = RenderBackendTextureClearValue::CreateColorValueUnit4(0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF);

    void RealTimeRenderer::Render(RenderGraph& renderGraph)
    {
        OPTICK_EVENT();

        const SceneView& view = *sceneView;

        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Create<RealTimeRendererSceneTextures>();
        //RealTimeRendererDebugViewModeTextures& debugViewModeTextures = renderGraph.blackboard.Create<RealTimeRendererDebugViewModeTextures>();

        RenderGraphTextureDesc vbuffer0Desc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::RG32Uint,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget,
            clearVisibilityBufferColor);
        sceneTextures.vbuffer0 = renderGraph.CreateTexture(vbuffer0Desc, "VBuffer0");

        RenderGraphTextureDesc vbuffer1Desc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::RGBA32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget,
            clearVisibilityBufferColor);
        sceneTextures.vbuffer1 = renderGraph.CreateTexture(vbuffer1Desc, "VBuffer1");

        RenderGraphTextureDesc gbuffer0Desc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::RGB10A2Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            clearColor);
        sceneTextures.gbuffer0 = renderGraph.CreateTexture(gbuffer0Desc, "GBuffer0");

        RenderGraphTextureDesc gbuffer1Desc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::RGBA8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            clearColor);
        sceneTextures.gbuffer1 = renderGraph.CreateTexture(gbuffer1Desc, "GBuffer1");

        RenderGraphTextureDesc gbuffer2Desc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::RGBA8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            clearColor);
        sceneTextures.gbuffer2 = renderGraph.CreateTexture(gbuffer2Desc, "GBuffer2");

        RenderGraphTextureDesc sceneColorTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget,
            clearColor,
            1,
            1,
            RenderBackendResourceState::UnorderedAccess);
        sceneTextures.sceneColorTexture = renderGraph.CreateTexture(sceneColorTextureDesc, "SceneColorTexture");

        RenderGraphTextureDesc sceneDepthTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::D32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::DepthStencil,
            clearDepth,
            1,
            1,
            RenderBackendResourceState::DepthStencil);
        sceneTextures.sceneDepthTexture = renderGraph.CreateTexture(sceneDepthTextureDesc, "SceneDepthTexture");

        RenderGraphTextureDesc motionVectorTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::RG16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        sceneTextures.motionVectorTexture = renderGraph.CreateTexture(motionVectorTextureDesc, "MotionVectorTexture");

        // RenderGraphTextureDesc finalTextureDesc = RenderGraphTextureDesc::Create2D(
        //     targetResolution.width,
        //     targetResolution.height,
        //     RenderBackendTextureFormat::RGB10A2Unorm,
        //     RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget | RenderBackendTextureCreateFlags::UnorderedAccess,
        //     RenderBackendTextureClearValue::None,
        //     1,
        //     1,
        //     RenderBackendResourceState::UnorderedAccess);
        // finalTextureData.finalTextureDesc = finalTextureDesc;
        // finalTextureData.finalTexture = renderGraph.CreateTexture(finalTextureDesc, "FinalTexture");
        //
        // ouptutTextureData.outputTexture = renderGraph.ImportExternalTexture(view.target, view.targetDesc, RenderBackendResourceState::Undefined, "CameraTarget");
        // ouptutTextureData.outputTextureDesc = view.targetDesc;

        // if (!historySceneDepthTextureCache.texture || (historySceneDepthTextureCache.desc != sceneDepthTextureDesc))
        // {
        //     historySceneDepthTextureCache.desc = sceneDepthTextureDesc;
        //     historySceneDepthTextureCache.texture = renderBackend->CreateTexture(deviceMask, &historySceneDepthTextureCache.desc, nullptr, "HistorySceneDepthTexture");
        //     historySceneDepthTextureCache.initialState = RenderBackendResourceState::DepthStencil;
        // }
        // historyInfo.historySceneDepthTexture = renderGraph.ImportExternalTexture(historySceneDepthTextureCache.texture, historySceneDepthTextureCache.desc, historySceneDepthTextureCache.initialState, "HistorySceneDepthTexture");
        // renderGraph.ExportTextureDeferred(sceneTextures.sceneDepthTexture, &historySceneDepthTextureCache);
        //
        // if (!historySceneColorTextureCache.texture || (historySceneColorTextureCache.desc != sceneColorTextureDesc))
        // {
        //     historySceneColorTextureCache.desc = sceneColorTextureDesc;
        //     historySceneColorTextureCache.texture = renderBackend->CreateTexture(deviceMask, &historySceneColorTextureCache.desc, nullptr, "HistorySceneColorTexture");
        //     historySceneColorTextureCache.initialState = RenderBackendResourceState::UnorderedAccess;
        // }
        // RenderGraphTextureHandle historySceneColor = renderGraph.ImportExternalTexture(historySceneColorTextureCache.texture, historySceneColorTextureCache.desc, historySceneColorTextureCache.initialState, "HistorySceneColorTexture");
        // renderGraph.ExportTextureDeferred(sceneTextures.sceneColorTexture, &historySceneColorTextureCache);

        //RenderVisibilityBuffer(renderGraph, view);

        //RenderGBuffer(renderGraph, view);

        RenderMotionVectors(renderGraph, view);

        // Hierarchical z-buffer must be aligned quad tree
        uint32 hzbWidth = Math::Max(Math::RoundUpToPowerOfTwo(renderResolution.width) >> 1, 1u);
        uint32 hzbHeight = Math::Max(Math::RoundUpToPowerOfTwo(renderResolution.height) >> 1, 1u);
        uint32 hzbMipLevels = Math::MaxNumMipLevels(hzbWidth, hzbHeight);

        RenderGraphTextureDesc hzbDesc = RenderGraphTextureDesc::Create2D(
            hzbWidth,
            hzbHeight,
            RenderBackendTextureFormat::R16Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource,
            RenderBackendTextureClearValue::DepthZero,
            hzbMipLevels);
        RenderGraphTextureHandle closestHZBTexture = renderGraph.CreateTexture(hzbDesc, "ClosestHZBTexture");
        RenderGraphTextureHandle furthestHZBTexture = renderGraph.CreateTexture(hzbDesc, "FurthestHZBTexture");

        //RenderDepthPyramid(renderGraph, view, hzbWidth, hzbHeight, hzbMipLevels, closestHZBTexture, furthestHZBTexture);

        if (IsSkyAtmosphereRenderingEnabled())
        {
            RenderSkyAtmosphereLUTs(renderGraph, view);
        }

        if (IsScreenSpaceAmbientOcclusionEnabled())
        {
            sceneTextures.ambientOcclusionTexture = RenderScreenSpaceAmbientOcclusion(renderGraph, view);
        }
        // if (IsRayTracingAmbientOcclusionEnabled())
        // {
        //     sceneTextures.ambientOcclusionTexture = RenderRayTracingAmbientOcclusion(renderGraph, view);
        // }

        //AddIndirectLightingDiffusePass(renderGraph, view);

        // RenderGraphTextureDesc reflectionsTextureDesc = RenderGraphTextureDesc::Create2D(
        //     renderResolution.width,
        //     renderResolution.height,
        //     RenderBackendTextureFormat::RGBA16Float,
        //     RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        // RenderGraphTextureHandle reflectionsTexture = renderGraph.CreateTexture(reflectionsTextureDesc, "ReflectionsTexture");
        //
        // RenderGraphTextureHandle ssrDebugOutputTexture = renderGraph.CreateTexture(RenderGraphTextureDesc::Create2D(
        //     renderResolution.width,
        //     renderResolution.height,
        //     RenderBackendTextureFormat::RGBA16Float,
        //     RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
        //     "SSRDebugOutputTexture");
        //
        // if (settings.reflectionsTechnique == ReflectionsTechnique::ScreenSpaceReflections)
        // {
        //   /*  RenderScreenSpaceReflections(
        //         renderGraph,
        //         view,
        //         hzbWidth,
        //         hzbHeight,
        //         closestHZBTexture,
        //         historySceneColor,
        //         historyInfo.historySceneDepth,
        //         ssrRayAllocationBuffer,
        //         reflectionsTexture,
        //         ssrDebugOutputTexture);*/
        // }
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
        //             return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //             {
        //                 commandList.ClearTextureUAV(
        //                     RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(reflectionsTexture), 0),
        //                     RenderBackendTextureClearValue::Black);
        //             };
        //         });
        // }

        //AddIndirectLightingSpecularPass(renderGraph, view);

        // AddSurfleGIPasses(renderGraph, view);

        // RenderGraphTextureDesc screenSpaceShadowMaskTextureDesc = RenderGraphTextureDesc::Create2D(
        //     renderResolution.width,
        //     renderResolution.height,
        //     RenderBackendTextureFormat::RGBA16Float,
        //     RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        // RenderGraphTextureHandle screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture");
        //
        // if (view.visualizationMode == SceneViewVisualizationMode::ShadowMask)
        // {
        //     debugViewModeTextures.screenSpaceShadowMaskTextureDesc = screenSpaceShadowMaskTextureDesc;
        //     debugViewModeTextures.screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture (Copy)");
        // }
        //
        // RenderGraphTextureDesc rayDistanceDesc = RenderGraphTextureDesc::Create2D(
        //     renderResolution.width,
        //     renderResolution.height,
        //     RenderBackendTextureFormat::R16Float,
        //     RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        // RenderGraphTextureHandle rayDistance = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "RayTracingShadowsRayDistance");
        //
        // for (uint32 lightIndex = 0; lightIndex < renderEngine->numLights; lightIndex++)
        // {
        //     if (renderEngine->lightData[lightIndex].type == (uint32)LightComponent::LightType::Directional)
        //     {
        //         if (ShouldRenderRayTracingShadowsForLight(*renderEngine->lightInfo[lightIndex].component))
        //         {
        //             RenderRayTracingShadows(renderGraph, view, *renderEngine->lightInfo[lightIndex].component, screenSpaceShadowMaskTexture, rayDistance);
        //         }
        //         else
        //         {
        //             RenderScreenSpaceShadows(renderGraph, view, *renderEngine->lightInfo[lightIndex].component, screenSpaceShadowMaskTexture);
        //         }
        //         //if (true)
        //         //{
        //         //    auto filteredShadowMask = renderGraph->CreateTexture(shadowMaskDesc, "FilteredShadowMask");
        //         //    DenoiseShadowMaskSSD(*renderGraph, blackboard, *view, filteredShadowMask, shadowMask);
        //         //}
        //         if (view.visualizationMode == SceneViewVisualizationMode::ShadowMask)
        //         {
        //             auto& debugViewModeTextures = renderGraph.blackboard.Get<RealTimeRendererDebugViewModeTextures>();
        //
        //             renderGraph.AddPass("CopyScreenSpaceShadowMaskTexture", RenderGraphPassFlags::Copy,
        //                 [&](RenderGraphBuilder& builder)
        //                 {
        //                     builder.ReadTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::CopySrc);
        //                     auto screenSpaceShadowMaskTextureCopy = debugViewModeTextures.screenSpaceShadowMaskTexture = builder.WriteTexture(debugViewModeTextures.screenSpaceShadowMaskTexture, RenderBackendResourceState::CopyDst);
        //
        //                     return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //                         {
        //                             Offset2D offset = { 0, 0 };
        //                             Extent2D extent = { debugViewModeTextures.screenSpaceShadowMaskTextureDesc.width, debugViewModeTextures.screenSpaceShadowMaskTextureDesc.height };
        //
        //                             commandList.CopyTexture2D(
        //                                 registry.GetRenderBackendTextureHandle(screenSpaceShadowMaskTexture),
        //                                 offset,
        //                                 0,
        //                                 registry.GetRenderBackendTextureHandle(screenSpaceShadowMaskTextureCopy),
        //                                 offset,
        //                                 0,
        //                                 extent);
        //                         };
        //                 });
        //         }
        //         break;
        //     }
        // }

        //RenderGraphTextureHandle localLightShadowMapAtlas = RenderLocalLightShadows(renderGraph, view);

        //AddDirectLightingPass(renderGraph, view, screenSpaceShadowMaskTexture, localLightShadowMapAtlas);
        renderGraph.AddPass(
            std::format("DirectLighting (Graphics, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);
                RenderGraphTextureHandle sceneDepthTexture = sceneTextures.sceneDepthTexture = builder.WriteTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);

                builder.BindRenderTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);
                builder.BindDepthStencil(sceneDepthTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(renderResolution.width), float(renderResolution.height));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                    commandList.SetScissors(&scissor, 1);
                };
            });

        if (IsSubsurfaceScatteringEnabled())
        {
            RenderSubsurfaceScattering(renderGraph, view);
        }

        if (IsSkyAtmosphereRenderingEnabled())
        {
            RenderSkyAtmosphere(renderGraph, view);
        }

        // const bool isVisualizeSufelEnabled = (view.visualizationMode == SceneViewVisualizationMode::SurfelGISurfel);
        //
        // if (isVisualizeSufelEnabled)
        // {
        //     AddSurfleGIVisualizationPass(renderGraph, view);
        // }

        if (IsScreenSpaceLightShaftsEnabled())
        {
            RenderScreenSpaceLightShafts(renderGraph, view);
        }

        RenderPostProcessingEffects(renderGraph, view);

        RenderGraphTextureHandle uiColorAndAlphaTexture = RenderUserInterface(renderGraph, view);

        renderGraph.AddPass(
            std::format("GUIComposition (Graphics, {}x{})", targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                uiColorAndAlphaTexture = builder.ReadTexture(uiColorAndAlphaTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle targetTexture = sceneTextures.hudLessColorTexture = builder.WriteTexture(sceneTextures.hudLessColorTexture, RenderBackendResourceState::RenderTarget);

                builder.BindRenderTarget(0, targetTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(targetResolution.width), float(targetResolution.height));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, targetResolution.width, targetResolution.height);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::SrcAlpha;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::OneMinusSrcAlpha;
                    graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcAlphaBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstAlphaBlendFactor = RenderBackendBlendFactor::OneMinusSrcAlpha;
                    graphicsPipelineState.colorBlendState.targetBlends[0].alphaBlendOp = RenderBackendBlendOp::Add;
                    graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::RGBA;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(uiColorAndAlphaTexture)));

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::FullScreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::GUICompositionPS);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderArguments,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });

        historyFrame.cameraJitterOffset = cameraJitterOffset;
        historyFrame.cameraPosition = view.cameraPosition;
        historyFrame.preExposure = preExposure;
        historyFrame.transformations = view.transformations;
    }
}