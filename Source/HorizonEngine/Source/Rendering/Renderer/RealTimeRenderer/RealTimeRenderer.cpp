#include "RealTimeRenderer.h"
#include "RealTimeRendererPrivate.h"
#include "PerFrameShaderParameters.h"
#include "SkyAtmosphereRendering.h"

#include <optick.h>

namespace Horizon
{
    static bool ShouldRenderRayTracingShadowsForLight(const LightComponent& light)
    {
        // Currently, only ray tracing shadows for directional lights are supoorted
        if (light.type != LightComponent::LightType::Directional || !light.UseRayTracingShadows())
        {
            return false;
        }
        return true;
    }

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
        if (autoExposureReadbackBuffer)
        {
            void* data = nullptr;
            renderBackend->MapBuffer(autoExposureReadbackBuffer->GetHandle(), &data);
            if (data != nullptr)
            {
                autoExposureData.adaptedExposure = ((float*)data)[0];
                autoExposureData.targetExposure = ((float*)data)[1];
                autoExposureData.exposureCompensation = ((float*)data)[2];
                autoExposureData.averageSceneLuminance = ((float*)data)[3];
                renderBackend->UnmapBuffer(autoExposureReadbackBuffer->GetHandle());
            }
        }
    }

    bool RealTimeRenderer::IsBloomEnabled() const
    {
        return IsGaussianBloomEnabled() || IsConvolutionBloomEnabled();
    }

    void RealTimeRenderer::OnRenderBegin()
    {
        // Super Resolution
        isDLSSEnabled = false;
        isFSR2Enabled = false;
        isSuperResolutionEnabled = false;
        switch (view.renderSettings.superResolutionTechnique)
        {
        case SuperResolutionTechnique::FSR2:
            {
                isFSR2Enabled = true;
                isSuperResolutionEnabled = true;
            } break;
        case SuperResolutionTechnique::DLSSSuperResolution:
            {
                isDLSSEnabled = true;
                isSuperResolutionEnabled = true;
            } break;
        default: break;
        }

        // Antialiasing
        isDLAAEnabled = false;
        isTemporalAAEnabled = false;
        if (!IsSuperResolutionEnabled())
        {
            switch (view.renderSettings.antialiasingTechnique)
            {
            case AntialiasingTechnique::TemporalAA:
                {
                    isTemporalAAEnabled = true;
                } break;
            case AntialiasingTechnique::DLAA:
                {
                    isDLAAEnabled = true;
                } break;
            default: break;
            }
        }

        upscaleRatio = 1.0f;

        renderResolution = view.swapChainBufferExtent;
        targetResolution = view.swapChainBufferExtent;
        displayResolution = view.swapChainBufferExtent;

        materialTextureMipLodBias = 0.0f;

        if (IsAutoMaterialTextureMipLodBiasEnabled())
        {
            materialTextureMipLodBias = std::log2f(float(renderResolution.width) / float(targetResolution.width));
        }
        // TODO: Override overrideMaterialTextureMipLodBias
        //if (overrideMaterialTextureMipLodBias)
        //{
        //
        //}

        Vector2 cameraJitterOffset = { 0.0f, 0.0f };
        if (ShouldApplyCameraJittering())
        {
            if (IsFSR2Enabled())
            {
                float jitterX = 0;
                float jitterY = 0;
                const int32 jitterPhaseCount = ffxFsr2GetJitterPhaseCount(renderResolution.width, targetResolution.width);
                FfxErrorCode errorCode = ffxFsr2GetJitterOffset(&jitterX, &jitterY, perFrameShaderParameters.frameIndex, jitterPhaseCount);
                FFX_ASSERT(errorCode == FFX_OK);

                Vector2 jitterOffset = { jitterX * 2.0f / (float)renderResolution.width, -jitterY * 2.0f / (float)renderResolution.height };

                jitteredProjectionMatrix[2][0] += -jitterOffset.x;
                jitteredProjectionMatrix[2][1] += -jitterOffset.y;

                cameraJitterOffset.x = jitterX;
                cameraJitterOffset.y = jitterY;
            }
            else
            {
                JitterProjectionMatrix(jitteredProjectionMatrix, cameraJitterOffset, renderResolution, upscaleRatio);
            }
        }
        auto jitteredInvProjectionMatrix = Math::InverseMatrix(jitteredProjectionMatrix);

        Vector2 previousCameraJitterOffset = perFrameShaderParameters.cameraJitterOffset;
        if (view.frameIndex == 0)
        {
            previousCameraJitterOffset = cameraJitterOffset;
        }

        Vector3 previousCameraPosition = perFrameShaderParameters.cameraPosition;

        enableFixedExposure = settings.exposureMethod == ExposureMethod::FixedExposure;

        float prevpreExposure = perFrameShaderParameters.preExposure;
        if (view.frameIndex == 0)
        {
            prevpreExposure = 1.0f;
        }

        float preExposure = 1.0f;
        if (view. != 0)
        {
            preExposure = preExposure;
        }
        if ()
        {
            preExposure = view.renderSettings.fixedPreExposure;
        }

        const RenderScene* scene = view.GetRenderScene();

        isSurfelGIEnabled = false;
        isRayTracingShadowsEnabled = settings.shadowsTechnique == ShadowsTechnique::RayTracingShadows;
        isRayTracingReflectionsEnabled = settings.reflectionsTechnique == ReflectionsTechnique::RayTracingReflections;
        isRayTracingAmbientOcclusionEnabled = settings.ambientOcclusionTechnique == AmbientOcclusionTechnique::RayTracingAmbientOcclusion;
        isSkyAtmosphereRenderingEnabled = IsSkyAtmosphereRenderingEnabled();

        features.enableSkyAtmosphereRendering =
            scene != nullptr &&
            scene->HasAtmosphericLight() &&
            scene->HasActiveSkyAtmosphere();

        bool isAutoExposureEnabled = (settings.exposureMethod == ExposureMethod::AutoExposure) || (settings.exposureMethod == ExposureMethod::FixedExposure);
        bool isBloomEnabled = settings.postProcessingSettings.bloomIntensity > 0.0f;
        bool isLensFlaresEnabled = isBloomEnabled && settings.postProcessingSettings.lensFlaresIntensity > 0.0f;
        bool isConvolutionBloomEnabled = false;
        bool isToneMappingEnabled = true;
        bool isLocalExposureEnabled = isToneMappingEnabled && settings.postProcessingSettings.localExposureEnabled;

        if (!IsAutoExposureEnabled())
        {
            finalPostProcessingSettings.autoExposureMinExposureValue = finalPostProcessingSettings.fixedExposureValue;
            finalPostProcessingSettings.autoExposureMinExposureValue = finalPostProcessingSettings.fixedExposureValue;
        }
    }

    void RealTimeRenderer::UpdatePerFrameDataBuffer() const
    {
        const SceneView& view = *sceneView;
        const RenderSettings& renderSettings = view.GetRenderSettings();

        // Setup PerFrameShaderParameters
        PerFrameShaderParameters perFrameShaderParameters = {};
        {
            perFrameShaderParameters.frameIndex = view.GetFrameIndex();

            perFrameShaderParameters.deltaTimeInSeconds = view.GetDeltaTimeInSeconds();

            perFrameShaderParameters.renderWidth = renderResolution.width;
            perFrameShaderParameters.renderHeight = renderResolution.height;
            perFrameShaderParameters.targetWidth = targetResolution.width;
            perFrameShaderParameters.targetHeight = targetResolution.height;
            perFrameShaderParameters.displayWidth = displayResolution.width;
            perFrameShaderParameters.displayHeight = displayResolution.height;

            perFrameShaderParameters.renderResolution = Vector4(1.0f * renderResolution.width, 1.0f * renderResolution.height, 1.0f / renderResolution.width, 1.0f / renderResolution.height);
            perFrameShaderParameters.targetResolution = Vector4(1.0f * targetResolution.width, 1.0f * targetResolution.height, 1.0f / targetResolution.width, 1.0f / targetResolution.height);
            perFrameShaderParameters.displayResolution = Vector4(1.0f * displayResolution.width, 1.0f * displayResolution.height, 1.0f / displayResolution.width, 1.0f / displayResolution.height);

            perFrameShaderParameters.cameraPosition = view.GetCameraPosition();
            perFrameShaderParameters.cameraJitterOffset = view.GetCameraJitterOffset();
            perFrameShaderParameters.cameraUpVector = view.GetCameraUpVector();
            perFrameShaderParameters.cameraRightVector = view.GetCameraRightVector();
            perFrameShaderParameters.cameraForwardVector = view.GetCameraForwardVector();
            perFrameShaderParameters.halfFovInRadians = Math::DegreesToRadians(view.GetFieldOfView()) * 0.5f;
            perFrameShaderParameters.aspectRatio = view.GetAspectRatio();
            perFrameShaderParameters.nearClippingPlane = view.GetNearClippingPlane();
            perFrameShaderParameters.farClippingPlane = view.GetFarClippingPlane();

            perFrameShaderParameters.previousCameraPosition = historyFrame.cameraPosition;
            perFrameShaderParameters.previousCameraJitterOffset = historyFrame.cameraJitterOffset;

            perFrameShaderParameters.worldToViewMatrix = view.GetTransformations().GetWorldToViewMatrix();
            perFrameShaderParameters.viewToWorldMatrix = view.GetTransformations().GetViewToWorldMatrix();
            perFrameShaderParameters.viewToClipMatrix = view.GetTransformations().GetViewToClipMatrix();
            perFrameShaderParameters.clipToViewMatrix = view.GetTransformations().GetClipToViewMatrix();
            perFrameShaderParameters.worldToClipMatrix = view.GetTransformations().GetWorldToClipMatrix();
            perFrameShaderParameters.clipToWorldMatrix = view.GetTransformations().GetClipToWorldMatrix();
            perFrameShaderParameters.nonJitteredWorldToClipMatrix = view.GetTransformations().GetNonJitteredViewToClipMatrix();

            perFrameShaderParameters.previousWorldToViewMatrix = historyFrame.transformations.GetWorldToViewMatrix();
            perFrameShaderParameters.previousViewToWorldMatrix = historyFrame.transformations.GetViewToWorldMatrix();
            perFrameShaderParameters.previousViewToClipMatrix = historyFrame.transformations.GetViewToClipMatrix();
            perFrameShaderParameters.previousClipToViewMatrix = historyFrame.transformations.GetClipToViewMatrix();
            perFrameShaderParameters.previousWorldToClipMatrix = historyFrame.transformations.GetWorldToClipMatrix();
            perFrameShaderParameters.previousClipToWorldMatrix = historyFrame.transformations.GetClipToWorldMatrix();
            perFrameShaderParameters.previousNonJitteredWorldToClipMatrix = historyFrame.transformations.GetNonJitteredViewToClipMatrix();

            perFrameShaderParameters.materialTextureMipLodBias = materialTextureMipLodBias;

            perFrameShaderParameters.preExposure = preExposure;
            perFrameShaderParameters.oneOverPreExposure = 1.0f / preExposure;
            perFrameShaderParameters.preExposureCorrection = preExposure / historyFrame.preExposure;

            perFrameShaderParameters.indirectLightingMultiplier = renderSettings.globalIlluminationSettings.indirectLightingIntensity * renderSettings.globalIlluminationSettings.indirectLightingColor;

            if (view.HasValidScene())
            {
                const RenderScene* scene = view.GetRenderScene();
                if (scene->HasAtmosphericLight())
                {
                    const DistantLightRenderProxy* atmosphericLight = scene->GetAtmosphericLight();
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
                    const SkyAtmosphereRenderProxy& skyAtmosphere = *scene->GetActiveSkyAtmosphere();

                    SkyAtmosphereShaderParameters skyAtmosphereShaderParameters = {};
                    SetupSkyAtmosphereShaderParameters(skyAtmosphereShaderParameters, skyAtmosphere);

                    SkyAtmosphereViewRelatedParameters skyAtmosphereViewRelatedParameters =
                    {
                        .skyViewLutReferential = IdentityMatrix3x3,
                    };
                    SetupSkyAtmosphereViewRelatedParameters(skyAtmosphereViewRelatedParameters, skyAtmosphere, view.GetCameraPosition(), view.GetCameraForwardVector());

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

    RenderGraphTextureHandle RealTimeRenderer::RenderUI(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        RenderGraphTextureHandle uiColorAndAlphaTexture = sceneTextures.uiColorAndAlphaTexture;

        renderGraph.AddPass(std::format("UIColorAndAlpha (Graphics, {}x{})", targetResolution.width, targetResolution.height), RenderGraphPassFlags::Graphics | RenderGraphPassFlags::SkipRenderPass,
        [&](RenderGraphBuilder& builder)
        {
            uiColorAndAlphaTexture = sceneTextures.uiColorAndAlphaTexture = builder.WriteTexture(sceneTextures.uiColorAndAlphaTexture, RenderBackendResourceState::RenderTarget);

            builder.BindColorTarget(0, uiColorAndAlphaTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

            return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
            {
                renderEngine->DrawUI(commandList, registry.GetRenderBackendTextureHandle(uiColorAndAlphaTexture));
            };
        });

        return uiColorAndAlphaTexture;
    }

    void RealTimeRenderer::Render(RenderGraph& renderGraph)
    {
        OPTICK_EVENT();

        uint32 deviceMask = ~0u;

        // TODO: move to other place
        shaderLibrary->HotReload();

        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Create<RealTimeRendererSceneTextures>();
        auto& historyInfo = renderGraph.blackboard.Create<RealTimeRendererHistoryInfo>();
        auto& finalTextureData = renderGraph.blackboard.Create<RenderGraphFinalTexture>();
        auto& ouptutTextureData = renderGraph.blackboard.Create<RenderGraphOutputTexture>();

        auto& debugViewModeTextures = renderGraph.blackboard.Create<RealTimeRendererDebugViewModeTextures>();

        RenderBackendTextureClearValue clearColor = RenderBackendTextureClearValue::CreateColorValueFloat4(0.0f, 0.0f, 0.0f, 0.0f);
        RenderBackendTextureClearValue clearDepth = RenderBackendTextureClearValue::CreateDepthValue(FarClipPlaneDepthValue);
        RenderBackendTextureClearValue clearVisibilityBufferColor = RenderBackendTextureClearValue::CreateColorValueUnit4(0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF);

        const bool shouldRenderSkyAtmosphere = ShouldRenderSkyAtmosphere();
        const bool shouldRenderAmbientOcclusion = settings.ambientOcclusionTechnique != AmbientOcclusionTechnique::None;

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
            RenderBackendTextureClearValue::None,
            1,
            1,
            RenderBackendResourceState::UnorderedAccess);
        sceneTextures.sceneColorTextureDesc = sceneColorTextureDesc;
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

        RenderGraphTextureDesc uiColorAndAlphaTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolution.width,
            targetResolution.height,
            RenderBackendTextureFormat::RGB10A2Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget);
        sceneTextures.uiColorAndAlphaTexture = renderGraph.CreateTexture(uiColorAndAlphaTextureDesc, "UIColorAndAlphaTexture");

        RenderGraphTextureDesc finalTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolution.width,
            targetResolution.height,
            RenderBackendTextureFormat::RGB10A2Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget | RenderBackendTextureCreateFlags::UnorderedAccess,
            RenderBackendTextureClearValue::None,
            1,
            1,
            RenderBackendResourceState::UnorderedAccess);
        finalTextureData.finalTextureDesc = finalTextureDesc;
        finalTextureData.finalTexture = renderGraph.CreateTexture(finalTextureDesc, "FinalTexture");

        ouptutTextureData.outputTexture = renderGraph.ImportExternalTexture(view.target, view.targetDesc, RenderBackendResourceState::Undefined, "CameraTarget");
        ouptutTextureData.outputTextureDesc = view.targetDesc;

        if (!historySceneDepthTextureCache.texture || (historySceneDepthTextureCache.desc != sceneDepthTextureDesc))
        {
            historySceneDepthTextureCache.desc = sceneDepthTextureDesc;
            historySceneDepthTextureCache.texture = renderBackend->CreateTexture(deviceMask, &historySceneDepthTextureCache.desc, nullptr, "HistorySceneDepthTexture");
            historySceneDepthTextureCache.initialState = RenderBackendResourceState::DepthStencil;
        }
        historyInfo.historySceneDepthTexture = renderGraph.ImportExternalTexture(historySceneDepthTextureCache.texture, historySceneDepthTextureCache.desc, historySceneDepthTextureCache.initialState, "HistorySceneDepthTexture");
        renderGraph.ExportTextureDeferred(sceneTextures.sceneDepthTexture, &historySceneDepthTextureCache);

        if (!historySceneColorTextureCache.texture || (historySceneColorTextureCache.desc != sceneColorTextureDesc))
        {
            historySceneColorTextureCache.desc = sceneColorTextureDesc;
            historySceneColorTextureCache.texture = renderBackend->CreateTexture(deviceMask, &historySceneColorTextureCache.desc, nullptr, "HistorySceneColorTexture");
            historySceneColorTextureCache.initialState = RenderBackendResourceState::UnorderedAccess;
        }
        RenderGraphTextureHandle historySceneColor = renderGraph.ImportExternalTexture(historySceneColorTextureCache.texture, historySceneColorTextureCache.desc, historySceneColorTextureCache.initialState, "HistorySceneColorTexture");
        renderGraph.ExportTextureDeferred(sceneTextures.sceneColorTexture, &historySceneColorTextureCache);

        RenderVisibilityBuffer(renderGraph, view);

        RenderGBuffer(renderGraph, view);

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

        RenderHZB(renderGraph, view, hzbWidth, hzbHeight, hzbMipLevels, closestHZBTexture, furthestHZBTexture);

        if (shouldRenderSkyAtmosphere)
        {
            RenderSkyAtmosphereLUTs(renderGraph);
        }

        if (settings.ambientOcclusionTechnique == AmbientOcclusionTechnique::GroundTruthAmbientOcclusion)
        {
            sceneTextures.ambientOcclusionTexture = RenderScreenSpaceAmbientOcclusion(renderGraph, view);
        }
        else if (settings.ambientOcclusionTechnique == AmbientOcclusionTechnique::RayTracingAmbientOcclusion)
        {
            sceneTextures.ambientOcclusionTexture = RenderRayTracingAmbientOcclusion(renderGraph, view);
        }
        else
        {
            sceneTextures.ambientOcclusionTexture = renderEngine->GetDefaultResources().ImportWhiteDummyTexture2D(renderGraph);
        }

        AddIndirectLightingDiffusePass(renderGraph, view);

        RenderGraphTextureDesc reflectionsTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle reflectionsTexture = renderGraph.CreateTexture(reflectionsTextureDesc, "ReflectionsTexture");

        RenderGraphTextureHandle ssrDebugOutputTexture = renderGraph.CreateTexture(RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
            "SSRDebugOutputTexture");

        if (settings.reflectionsTechnique == ReflectionsTechnique::ScreenSpaceReflections)
        {
          /*  RenderScreenSpaceReflections(
                renderGraph,
                view,
                hzbWidth,
                hzbHeight,
                closestHZBTexture,
                historySceneColor,
                historyInfo.historySceneDepth,
                ssrRayAllocationBuffer,
                reflectionsTexture,
                ssrDebugOutputTexture);*/
        }
        else if (settings.reflectionsTechnique == ReflectionsTechnique::RayTracingReflections)
        {
            //RenderRayTracingReflections();
        }
        else
        {
            renderGraph.AddPass("ClearReflectionTexture", RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    reflectionsTexture = builder.WriteTexture(reflectionsTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        commandList.ClearTextureUAV(
                            RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(reflectionsTexture), 0),
                            RenderBackendTextureClearValue::Black);
                    };
                });
        }

        AddIndirectLightingSpecularPass(renderGraph, view);

        // AddSurfleGIPasses(renderGraph, view);

        RenderGraphTextureDesc screenSpaceShadowMaskTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture");

        if (view.visualizationMode == SceneViewVisualizationMode::ShadowMask)
        {
            debugViewModeTextures.screenSpaceShadowMaskTextureDesc = screenSpaceShadowMaskTextureDesc;
            debugViewModeTextures.screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture (Copy)");
        }

        RenderGraphTextureDesc rayDistanceDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle rayDistance = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "RayTracingShadowsRayDistance");

        for (uint32 lightIndex = 0; lightIndex < renderEngine->numLights; lightIndex++)
        {
            if (renderEngine->lightData[lightIndex].type == (uint32)LightComponent::LightType::Directional)
            {
                if (ShouldRenderRayTracingShadowsForLight(*renderEngine->lightInfo[lightIndex].component))
                {
                    RenderRayTracingShadows(renderGraph, view, *renderEngine->lightInfo[lightIndex].component, screenSpaceShadowMaskTexture, rayDistance);
                }
                else
                {
                    RenderScreenSpaceShadows(renderGraph, view, *renderEngine->lightInfo[lightIndex].component, screenSpaceShadowMaskTexture);
                }
                //if (true)
                //{
                //    auto filteredShadowMask = renderGraph->CreateTexture(shadowMaskDesc, "FilteredShadowMask");
                //    DenoiseShadowMaskSSD(*renderGraph, blackboard, *view, filteredShadowMask, shadowMask);
                //}
                if (view.visualizationMode == SceneViewVisualizationMode::ShadowMask)
                {
                    auto& debugViewModeTextures = renderGraph.blackboard.Get<RealTimeRendererDebugViewModeTextures>();

                    renderGraph.AddPass("CopyScreenSpaceShadowMaskTexture", RenderGraphPassFlags::Copy,
                        [&](RenderGraphBuilder& builder)
                        {
                            builder.ReadTexture(screenSpaceShadowMaskTexture, RenderBackendResourceState::CopySrc);
                            auto screenSpaceShadowMaskTextureCopy = debugViewModeTextures.screenSpaceShadowMaskTexture = builder.WriteTexture(debugViewModeTextures.screenSpaceShadowMaskTexture, RenderBackendResourceState::CopyDst);

                            return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                                {
                                    Offset2D offset = { 0, 0 };
                                    Extent2D extent = { debugViewModeTextures.screenSpaceShadowMaskTextureDesc.width, debugViewModeTextures.screenSpaceShadowMaskTextureDesc.height };

                                    commandList.CopyTexture2D(
                                        registry.GetRenderBackendTextureHandle(screenSpaceShadowMaskTexture),
                                        offset,
                                        0,
                                        registry.GetRenderBackendTextureHandle(screenSpaceShadowMaskTextureCopy),
                                        offset,
                                        0,
                                        extent);
                                };
                        });
                }
                break;
            }
        }

        RenderGraphTextureHandle localLightShadowMapAtlas = RenderLocalLightShadows(renderGraph, view);

        //renderGraph.AddPass("Sky", RenderGraphPassFlags::Graphics,
        //    [&](RenderGraphBuilder& builder)
        //    {
        //        auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::DepthStencil);
        //        auto sceneColorTexture = sceneTextures.sceneColorTexture = builder.ReadWriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);

        //        builder.BindColorTarget(0, sceneColorTexture, RenderBackendRenderTargetLoadOp::Load, RenderBackendRenderTargetStoreOp::Store);
        //        builder.BindDepthTarget(sceneDepthTexture, RenderBackendRenderTargetLoadOp::Load, RenderBackendRenderTargetStoreOp::Store);

        //        return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //        {
        //            uint32 threadGroupCountX = ComputeWorkGroupCount(renderResolution.width, 8);
        //            uint32 threadGroupCountY = ComputeWorkGroupCount(renderResolution.height, 8);

        //            RenderBackendGraphicsPipelineState graphicsPipelineState = {};
        //            graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
        //            graphicsPipelineState.depthStencilState.depthTestEnable = true;
        //            graphicsPipelineState.depthStencilState.depthWriteEnable = false;
        //            graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::Equal;

        //            RenderBackendShaderArguments shaderArguments = {};
        //            shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
        //            shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(renderEngine->environmentMap));
        //            shaderArguments.PushConstants(0, 0.0f);

        //            RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::SkyBox);
        //            commandList.Draw(
        //                graphicsShader,
        //                graphicsPipelineState,
        //                shaderArguments,
        //                3, 1, 0, 0,
        //                RenderBackendPrimitiveTopology::TriangleList);
        //        };
        //    });

        AddDirectLightingPass(
            renderGraph,
            view,
            screenSpaceShadowMaskTexture,
            localLightShadowMapAtlas);

        // TODO
        const bool shouldRenderSubsurfaceScattering = false;

        if (IsS)
        {
            RenderSubsurfaceScattering(renderGraph, view);
        }

        if (IsSkyAtmosphereRenderingEnabled())
        {
            RenderSkyAtmosphere(renderGraph, *skyAtmosphere, *(renderEngine->skyAtmosphereComponent), renderResolution.width, renderResolution.height, sceneViewShaderParametersBuffer);
        }

        const bool isVisualizeSufelEnabled = (view.visualizationMode == SceneViewVisualizationMode::SurfelGISurfel);

        if (isVisualizeSufelEnabled)
        {
            AddSurfleGIVisualizationPass(renderGraph, view);
        }

        const bool shouldRenderLightShafts = true; // ShouldRenderLightShafts();
        if (shouldRenderLightShafts)
        {
            RenderLightShafts(renderGraph, view);
        }

        AddPostProcessingPasses(renderGraph, view);

        RenderUIColorAndAlpha(renderGraph, view);

        renderGraph.AddPass(std::format("FinalComposite (Graphics, {}x{})", targetResolution.width, targetResolution.height), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                auto& finalColorTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();

                auto uiColorAndAlphaTexture = builder.ReadTexture(sceneTextures.uiColorAndAlphaTexture, RenderBackendResourceState::ShaderResource);
                auto finalColorTexture = finalColorTextureData.finalTexture = builder.WriteTexture(finalColorTextureData.finalTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, finalColorTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, (float)targetResolution.width, (float)targetResolution.height);
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
                    graphicsPipelineState.colorBlendState.targetBlends[0].colorWriteMask = RenderBackendColorComponentFlags::RGBA;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(uiColorAndAlphaTexture)));

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::GUIComposition);
                    commandList.Draw(
                        graphicsShader,
                        graphicsPipelineState,
                        shaderArguments,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });

        renderGraph.AddPass("Present", RenderGraphPassFlags::NeverGetCulled,
            [&](RenderGraphBuilder& builder)
            {
                auto& finalTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();
                auto& outputTextureData = renderGraph.blackboard.Get<RenderGraphOutputTexture>();

                auto finalTexture = builder.ReadTexture(finalTextureData.finalTexture, RenderBackendResourceState::CopySrc);
                auto outputTexture = outputTextureData.outputTexture = builder.WriteTexture(outputTextureData.outputTexture, RenderBackendResourceState::CopyDst);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    Offset2D offset = { 0, 0 };
                    Extent2D extent = { targetResolution.width, targetResolution.height };

                    commandList.CopyTexture2D(
                        registry.GetRenderBackendTextureHandle(finalTexture),
                        offset,
                        0,
                        registry.GetRenderBackendTextureHandle(outputTexture),
                        offset,
                        0,
                        extent);

                    RenderBackendBarrier transitions[] =
                    {
                        RenderBackendBarrier(registry.GetRenderBackendTextureHandle(outputTexture), RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::CopyDst, RenderBackendResourceState::Present)
                    };
                    commandList.Transitions(transitions, 1);
                };
            });
    }

    bool RealTimeRenderer::IsSuperResolutionEnabled() const
    {
        return features.enableSuperResolution;
    }
}