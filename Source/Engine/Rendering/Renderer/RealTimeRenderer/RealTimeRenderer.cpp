#include "RealTimeRenderer.h"

#include "Rendering/RenderAPI.h"

#include <ffx_fsr2.h>

#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>

#include <optick.h>

namespace HE
{
    void JitterProjectionMatrix(Matrix4x4& outProjectionMatrix, Vector2& outJitterOffset, const Extent2D& renderResolution, float upscaleRatio)
    {
        static const auto HaltonSequence = [](uint32 index, uint32 base)
        {
            float f = 1.0f, result = 0.0f;
            for (uint32 i = index; i > 0;)
            {
                f /= static_cast<float>(base);
                result = result + f * static_cast<float>(i % base);
                i = static_cast<uint32>(floorf(static_cast<float>(i) / static_cast<float>(base)));
            }
            return result;
        };

        uint32 temporalSampleCount = 32;
        temporalSampleCount = uint32(float(temporalSampleCount) * std::max(1.0f, upscaleRatio * upscaleRatio));
        temporalSampleCount = Math::Clamp(temporalSampleCount, 1, 255);

        static uint32 temporalSampleIndex = 0;
        temporalSampleIndex = (temporalSampleIndex + 1) % temporalSampleCount;

        // Unit pixel space offset
        float offsetX = HaltonSequence(temporalSampleIndex + 1, 2) - 0.5f;
        float offsetY = HaltonSequence(temporalSampleIndex + 1, 3) - 0.5f;

        // Clip space offset [-1, 1]
        // -y for clip space to uv space
        Vector2 jitterOffset = { offsetX * 2.0f / (float)renderResolution.width, -offsetY * 2.0f / (float)renderResolution.height };

        /*
         * Horizon Engine uses righted-handed coordinate system,
         * the w component of clip space position is -Zc instead of Zc,
         * so it should be multiplied by -1.
         */
        outProjectionMatrix[2][0] += -jitterOffset.x;
        outProjectionMatrix[2][1] += -jitterOffset.y;
        outJitterOffset = { offsetX, offsetY };
    }

    static bool ShouldRenderRayTracingShadowsForLight(const LightComponent& light)
    {
#if HE_DISABLE_HARDWARE_RAY_TRACING
        return false;
#else
        // Currently, only ray tracing shadows for directional lights are supoorted
        if (light.type != LightComponent::LightType::Directional || !light.UseRayTracingShadows())
        {
            return false;
        }
        return true;
#endif
    }

    RealTimeRenderer::RealTimeRenderer(RenderBackend* backend, ShaderCompiler* compiler, RenderSystem* renderEngine)
        : renderBackend(backend)
        , shaderCompiler(compiler)
        , shaderLibrary(nullptr)
        , renderEngine(renderEngine)
    {
        additiveRGBAColorBlendAttachmentState.blendEnable = true;
        additiveRGBAColorBlendAttachmentState.srcColorBlendFactor = RenderBackendBlendFactor::One;
        additiveRGBAColorBlendAttachmentState.dstColorBlendFactor = RenderBackendBlendFactor::One;
        additiveRGBAColorBlendAttachmentState.colorBlendOp = RenderBackendBlendOp::Add;
        additiveRGBAColorBlendAttachmentState.srcAlphaBlendFactor = RenderBackendBlendFactor::One;
        additiveRGBAColorBlendAttachmentState.dstAlphaBlendFactor = RenderBackendBlendFactor::One;
        additiveRGBAColorBlendAttachmentState.alphaBlendOp = RenderBackendBlendOp::Add;
        additiveRGBAColorBlendAttachmentState.colorWriteMask = RenderBackendColorComponentFlags::RGBA;

        additiveRGBColorBlendAttachmentState.blendEnable = true;
        additiveRGBColorBlendAttachmentState.srcColorBlendFactor = RenderBackendBlendFactor::One;
        additiveRGBColorBlendAttachmentState.dstColorBlendFactor = RenderBackendBlendFactor::One;
        additiveRGBColorBlendAttachmentState.colorBlendOp = RenderBackendBlendOp::Add;
        additiveRGBColorBlendAttachmentState.colorWriteMask = RenderBackendColorComponentFlags::RGB;

        // TODO: Remove this
        settings.ambientOcclusionTechnique = AmbientOcclusionTechnique::GroundTruthAmbientOcclusion;
        settings.antialiasingTechnique = AntialiasingTechnique::TemporalAA;

        settings.postProcessingSettings.fixedExposureValue = 0.0f;

        settings.postProcessingSettings.dofScale = 0.0f;
        settings.postProcessingSettings.dofFocalDistance = 5.0f;
        settings.postProcessingSettings.dofFocalRegion = 1.0f;
        settings.postProcessingSettings.dofNearTransitionRegion = 0.0f;
        settings.postProcessingSettings.dofFarTransitionRegion = 0.0f;
        settings.postProcessingSettings.dofNearRegionBlurSize = 1.0f;
        settings.postProcessingSettings.dofFarRegionBlurSize = 1.0f;

        settings.postProcessingSettings.localExposureShadows = 1.5f;
        settings.postProcessingSettings.localExposureHighlights = 2.0f;
        settings.postProcessingSettings.localExposureCoarsestMipLevel = 2;
        settings.postProcessingSettings.localExposureDisplayMipLevel = 1;
        settings.postProcessingSettings.localExposurePreferenceSigma = 5.0f;

        settings.postProcessingSettings.bloomIntensity = 1.0f;
        settings.postProcessingSettings.bloomRadius = 0.5f;
        settings.postProcessingSettings.lensDirtIntensity = 0.0f;
        settings.postProcessingSettings.lensDirtTint = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

        settings.postProcessingSettings.lensFlaresIntensity = 0.0f;

        settings.postProcessingSettings.colorGradingWhiteBalanceColorTemperature = 6500.0f;

        settings.postProcessingSettings.chromaticAberrationIntensity = 0.0f;
        settings.postProcessingSettings.chromaticAberrationOffset = 0.0f;

        settings.postProcessingSettings.localExposureEnabled = false;

        settings.postProcessingSettings.colorCorrectionSaturation = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        settings.postProcessingSettings.colorCorrectionContrast = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        settings.postProcessingSettings.colorCorrectionGamma = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        settings.postProcessingSettings.colorCorrectionGain = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        settings.postProcessingSettings.colorCorrectionOffset = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

        shaderLibrary = new ShaderLibrary_Deprecated(renderBackend, shaderCompiler, (uint32)RealTimeRendererShaderPiplineID::Count, true);
        shaderLibrary->AddIncludeDirectory("../../../Shaders");
        shaderLibrary->AddIncludeDirectory("../../../Shaders/RealTimeRenderer");
        shaderLibrary->AddIncludeDirectory("../../../Shaders/RealTimeRenderer/SubsurfaceScattering");
        shaderLibrary->AddIncludeDirectory("../../../Shaders/RealTimeRenderer/SurfelGI");
        shaderLibrary->AddIncludeDirectory("../../../Shaders/RealTimeRenderer/PostProcessing");

        uint32 deviceMask = ~0u;

        RenderBackendSamplerDesc samplerLinearWarpDesc = RenderBackendSamplerDesc::CreateLinearWarp(0.0f, -FLOAT_MAX, FLOAT_MAX, 1);
        samplerLinearWarp = renderBackend->CreateSampler(deviceMask, &samplerLinearWarpDesc, "SamplerLinearWarp");
        RenderBackendSamplerDesc samplerLinearClampDesc = RenderBackendSamplerDesc::CreateLinearClamp(0.0f, -FLOAT_MAX, FLOAT_MAX, 1);
        samplerLinearClamp = renderBackend->CreateSampler(deviceMask, &samplerLinearClampDesc, "SamplerLinearClamp");
        RenderBackendSamplerDesc samplerLinearBorderDesc = RenderBackendSamplerDesc::CreateLinearBorder(0.0f, -FLOAT_MAX, FLOAT_MAX, 1);
        samplerLinearBorder = renderBackend->CreateSampler(deviceMask, &samplerLinearBorderDesc, "SamplerLinearBorder");
        RenderBackendSamplerDesc samplerPointWarpDesc = RenderBackendSamplerDesc::CreatePointWarp(0.0f, -FLOAT_MAX, FLOAT_MAX, 1);
        samplerPointWarp = renderBackend->CreateSampler(deviceMask, &samplerPointWarpDesc, "SamplerPointWarp");
        RenderBackendSamplerDesc samplerPointClampDesc = RenderBackendSamplerDesc::CreatePointClamp(0.0f, -FLOAT_MAX, FLOAT_MAX, 1);
        samplerPointClamp = renderBackend->CreateSampler(deviceMask, &samplerPointClampDesc, "SamplerPointClamp");
        RenderBackendSamplerDesc samplerPointBorderDesc = RenderBackendSamplerDesc::CreatePointBorder(0.0f, -FLOAT_MAX, FLOAT_MAX, 1);
        samplerPointBorder = renderBackend->CreateSampler(deviceMask, &samplerPointBorderDesc, "SamplerPointBorder");

        RenderBackendSamplerDesc samplerComparisonGreaterLinearClampDesc = RenderBackendSamplerDesc::CreateComparisonLinearClamp(0.0f, -FLOAT_MAX, FLOAT_MAX, 1, RenderBackendCompareOp::Greater);
        samplerComparisonGreaterLinearClamp = renderBackend->CreateSampler(deviceMask, &samplerComparisonGreaterLinearClampDesc, "SamplerComparisonGreaterLinearClamp");
        RenderBackendSamplerDesc samplerCmpDepthDesc = RenderBackendSamplerDesc::CreateComparisonLinearClamp(0.0f, -FLOAT_MAX, FLOAT_MAX, 0, RenderBackendCompareOp::Less);
        samplerCmpDepth = renderBackend->CreateSampler(deviceMask, &samplerCmpDepthDesc, "SamplerCmpDepth");

        for (uint32 index = 0; index < NumAutoExposureReadbackBuffers; index++)
        {
            autoExposureReadbackBuffers[index].name = "AutoExposureReadBackBuffer";
            autoExposureReadbackBuffers[index].active = true;
            autoExposureReadbackBuffers[index].desc = RenderBackendBufferDesc::CreateReadback(sizeof(float));
            autoExposureReadbackBuffers[index].buffer = renderBackend->CreateBuffer(deviceMask, &autoExposureReadbackBuffers[index].desc, nullptr, autoExposureReadbackBuffers[index].name.c_str());
            autoExposureReadbackBuffers[index].initialState = RenderBackendResourceState::Undefined;
        }

        RenderBackendBufferDesc surfelGIInfoBufferDesc = RenderBackendBufferDesc::CreateByteAddress(SurfelGIInfoBufferSize);
        surfelGIInfoBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGIInfoBufferDesc, nullptr, "SurfelGIInfoBuffer");

        RenderBackendBufferDesc surfelGIArgumentBufferDesc = RenderBackendBufferDesc::CreateIndirectArguments(sizeof(uint32) * 12);
        surfelGIArgumentBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGIArgumentBufferDesc, nullptr, "SurfelGIArgumentBuffer");

        RenderBackendBufferDesc surfelGISurfelIndirectionBufferDesc = RenderBackendBufferDesc::CreateByteAddress(SurfelGIMaxSurfelCount * sizeof(uint32));
        surfelGIAliveSurfelIndirectionBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGISurfelIndirectionBufferDesc, nullptr, "SurfelGIAliveSurfelIndirectionBuffer");
        surfelGIFreeSurfelBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGISurfelIndirectionBufferDesc, nullptr, "SurfelGIFreeSurfelBuffer");

        RenderBackendBufferDesc surfelGISurfelHotDataBufferDecs = RenderBackendBufferDesc::CreateByteAddress(SurfelGIMaxSurfelCount * sizeof(SurfelHotData));
        surfelGISurfelHotDataBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGISurfelHotDataBufferDecs, nullptr, "SurfelGISurfelHotDataBuffer");

        RenderBackendBufferDesc surfelGICellHeaderBufferDesc = RenderBackendBufferDesc::CreateByteAddress(SurfelGIUniformGridCellCount * sizeof(uint32) * 2);
        surfelGICellHeaderBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGICellHeaderBufferDesc, nullptr, "SurfelGICellHeaderBuffer");

        RenderBackendBufferDesc surfelGICellDataBufferDesc = RenderBackendBufferDesc::CreateByteAddress(SurfelGIMaxSurfelCount * sizeof(uint32));
        surfelGICellDataBuffer = renderBackend->CreateBuffer(deviceMask, &surfelGICellDataBufferDesc, nullptr, "SurfelGICellDataBuffer");

        RenderBackendBufferDesc sceneViewShaderParametersBufferDesc = RenderBackendBufferDesc::CreateByteAddress(sizeof(RealTimeRendererSceneViewShaderParameters));
        sceneViewShaderParametersBuffer = renderBackend->CreateBuffer(deviceMask, &sceneViewShaderParametersBufferDesc, nullptr, "SceneViewShaderParametersBuffer");

        RenderBackendBufferDesc sceneViewShaderParametersUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(sizeof(RealTimeRendererSceneViewShaderParameters));
        sceneViewShaderParametersUploadBuffer = renderBackend->CreateBuffer(deviceMask, &sceneViewShaderParametersUploadBufferDesc, nullptr, "SceneViewShaderParametersUploadBuffer");

        //testTexture = LoadTextureFromFile(renderBackend, "../../../Assets/PurkinjeShift.png", false);

        RenderBackendBufferDesc ssrRayAllocationBufferDesc = RenderBackendBufferDesc::CreateIndirectArguments(sizeof(uint32), 12);
        ssrRayAllocationBuffer = renderBackend->CreateBuffer(deviceMask, &ssrRayAllocationBufferDesc, nullptr, "SSRRayAllocationBuffer");

        defaultBloomKernelTexture = LoadTextureFromHDRFile(renderBackend, "../../../Assets/HDRIs/DefaultBloomKernel.hdr", &defaultBloomKernelTextureDesc);

        localExposureTestTexture = LoadTextureFromHDRFile(renderBackend, "../../../Assets/HDRIs/veranda_2k.hdr", &localExposureTestTextureDesc);

        lensDirtTexture = LoadTextureFromFile(GRenderBackend, "../../../Assets/Textures/LensDirtTexture.png", false);
        lensFlaresGlareLUTTexture = LoadTextureFromFile(GRenderBackend, "../../../Assets/Textures/LensFlaresGlareLUT.png", false);
        lensFlaresGradiantLUTTexture = LoadTextureFromFile(GRenderBackend, "../../../Assets/Textures/LensFlaresGradiantLUT.png", false);

        blueNoiseTexture = LoadTextureFromFile(GRenderBackend, "../../../Assets/Textures/BlueNoise.png", false);
        {
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
        }
        renderBackend->FlushRenderDevices();

        LoadShaders();
    }

    RealTimeRenderer::~RealTimeRenderer()
    {
        delete shaderLibrary;
    }

    bool RealTimeRenderer::LoadShaders()
    {
        bool result = true;

        uint32 deviceMask = ~0u;

        ShaderDesc shaderDesc;

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/VisibilityBuffer.hsf", "VisibilityBufferVS", "VisibilityBufferPS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::VBuffer, shaderDesc);

        shaderDesc = ShaderDesc::CreateMesh("RealTimeRenderer/VisibilityBufferMeshlet.hsf", "VisibilityBufferTS", "VisibilityBufferMS", "VisibilityBufferPS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::VBufferMeshlet, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/BuildHZB.hsf", "BuildHZBCS");
        shaderDesc.AddDefine("BUILD_CLOSEST_HZB", 1);
        shaderDesc.AddDefine("BUILD_FURTHEST_HZB", 1);
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::BuildHZB, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/ShadowMap.hsf", "ShadowMapVS", "ShadowMapPS");
        shaderDesc.AddDefine("SHADOW_MAP_TYPE", 0);
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::CascadedShadowMap, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/ShadowMap.hsf", "ShadowMapVS", "ShadowMapPS");
        shaderDesc.AddDefine("SHADOW_MAP_TYPE", 1);
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::CubeShadowMap, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/LocalLightShadows.hsf", "LocalLightShadowsVS", "LocalLightShadowsPS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LocalLightShadows, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceShadows.hsf", "ScreenSpaceShadowsDirectionalLightCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::ScreenSpaceShadowsDirectionalLight, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/GBuffer.hsf", "GBufferCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::GBuffer, shaderDesc);

        //{
        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflectionsTileClassification.hsf", "SSRTileClassificationHorizontalCS");
        //    result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SSRTileClassificationHorizontal, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflectionsTileClassification.hsf", "SSRTileClassificationVerticalCS");
        //    result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SSRTileClassificationVertical, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflectionsRayAllocation.hsf", "SSRRayAllocationCS");
        //    result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SSRRayAllocation, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflections.hsf", "ScreenSpaceReflectionsCS");
        //    shaderDesc.AddDefine(HE_TEXT("SSR_EARLY_EXIT_RAYS"));
        //    result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SSRDispatchEarlyExitRays, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflections.hsf", "ScreenSpaceReflectionsCS");
        //    shaderDesc.AddDefine(HE_TEXT("SSR_CHEAP_RAYS"));
        //    result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SSRDispatchCheapRays, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflections.hsf", "ScreenSpaceReflectionsCS");
        //    result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SSRDispatchExpensiveRays, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflectionsResolve.hsf", "ScreenSpaceReflectionsResolveCS");
        //    result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SSRResolve, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflectionsTemporalFiltering.hsf", "SSRTemporalFilteringCS");
        //    result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SSRTemporalFiltering, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflectionsSpatialFiltering.hsf", "SSRSpatialFilteringCS");
        //    result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SSRSpatialFiltering, shaderDesc);
        //}

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/GTAOHorizonSearchAndIntegral.hsf", "GTAOHorizonSearchAndIntegralCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::GTAOHorizonSearchAndIntegral, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/GTAOSpatialFiltering.hsf", "GTAOSpatialFilteringCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::GTAOSpatialFiltering, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceAmbientOcclusion.hsf", "GTAOTemporalFilteringCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::GTAOTemporalFiltering, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/IndirectLightingDiffuse.hsf", "IndirectLightingDiffuseVS", "IndirectLightingDiffusePS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::IndirectLightingDiffuse, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/IndirectLightingSpecular.hsf", "IndirectLightingSpecularVS", "IndirectLightingSpecularPS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::IndirectLightingSpecular, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/MotionVectors.hsf", "MotionVectorsCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::MotionVectors, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/DirectLighting.hsf", "DirectLightingVS", "DirectLightingPS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::DirectLighting, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/SkyBox.hsf", "SkyBoxVS", "SkyBoxPS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SkyBox, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringSetup.hsf", "SubsurfaceScatteringSetupCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SubsurfaceScatteringSetup, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringClassifyTiles.hsf", "SubsurfaceScatteringClassifyTilesCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SubsurfaceScatteringClassifyTiles, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringBuildIndirectArguments.hsf", "SubsurfaceScatteringBuildIndirectArgumentsCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SubsurfaceScatteringBuildIndirectArguments, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringSampleDiffusionProfile.hsf", "SubsurfaceScatteringSampleDiffusionProfileCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SubsurfaceScatteringSampleDiffusionProfile, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringComputeVariance.hsf", "SubsurfaceScatteringComputeVarianceCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SubsurfaceScatteringComputeVariance, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringCopyResults.hsf", "SubsurfaceScatteringCopyResultsVS", "SubsurfaceScatteringCopyResultsPS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SubsurfaceScatteringCopyResults, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringRecombine.hsf", "SubsurfaceScatteringRecombineVS", "SubsurfaceScatteringRecombinePS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SubsurfaceScatteringRecombine, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIFreeSurfels.hsf", "SurfelGIFreeSurfelsCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SurfelGIFreeSurfels, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIGapFilling.hsf", "SurfelGIGapFillingCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SurfelGIGapFilling, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIIndirectAruguments.hsf", "SurfelGIIndirectArugumentsCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SurfelGIIndirectAruguments, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIGridReset.hsf", "SurfelGIGridResetCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SurfelGIGridReset, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIComputeCellCapacity.hsf", "SurfelGIComputeCellCapacityCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SurfelGIComputeCellCapacity, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIComputeCellOffset.hsf", "SurfelGIComputeCellOffsetCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SurfelGIComputeCellOffset, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIVisualization.hsf", "SurfelGIVisualizationCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::SurfelGIVisualization, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/LightShaftsDownsample.hsf", "LightShaftsDownsampleCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LightShaftsDownsample, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/LightShaftsRadialBlur.hsf", "LightShaftsRadialBlurCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LightShaftsRadialBlur, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/LightShaftsApply.hsf", "LightShaftsApplyCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LightShaftsApply, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/TemporalSuperSampling.hsf", "TemporalSuperSamplingCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::TemporalSuperSampling, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/DepthOfFieldSetup.hsf", "DepthOfFieldSetupCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::DepthOfFieldSetup, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/DepthOfFieldGather.hsf", "DepthOfFieldGatherCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::DepthOfFieldGather, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/DepthOfFieldPostfilter.hsf", "DepthOfFieldPostfilterCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::DepthOfFieldPostfilter, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/DepthOfFieldRecombine.hsf", "DepthOfFieldRecombineCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::DepthOfFieldRecombine, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/AutoExposureBuildHistogram.hsf", "AutoExposureBuildHistogramCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::AutoExposureBuildHistogram, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/AutoExposureComputeExposure.hsf", "AutoExposureComputeExposureCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::AutoExposureComputeExposure, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/Downsample.hsf", "DownsampleCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::Downsample, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/GaussianBloomDownsample.hsf", "GaussianBloomDownsampleCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::GaussianBloomDownsample, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/GaussianBloomUpsample.hsf", "GaussianBloomUpsampleCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::GaussianBloomUpsample, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/ConvolutionBloomResizeKernel.hsf", "ConvolutionBloomResizeKernelCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::ConvolutionBloomResizeKernel, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/PostProcessing/LensFlaresGhost.hsf", "LensFlaresGhostVS", "LensFlaresGhostPS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LensFlaresGhost, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/LensFlaresTileCulling.hsf", "LensFlaresTileCullingCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LensFlaresTileCulling, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/PostProcessing/LensFlaresGlare.hsf", "LensFlaresGlareVS", "LensFlaresGlarePS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LensFlaresGlare, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/PostProcessing/LensFlaresCombine.hsf", "LensFlaresCombineVS", "LensFlaresCombinePS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LensFlaresCombine, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/LocalExposureComputeLuminances.hsf", "LocalExposureComputeLuminancesCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LocalExposureComputeLuminances, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/LocalExposureComputeWeights.hsf", "LocalExposureComputeWeightsCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LocalExposureComputeWeights, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/LocalExposureBlendExposures.hsf", "LocalExposureBlendExposuresCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LocalExposureBlendExposures, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/LocalExposureBlendLaplacian.hsf", "LocalExposureBlendLaplacianCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LocalExposureBlendLaplacian, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/LocalExposureGuidedUpsampling.hsf", "LocalExposureGuidedUpsamplingCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::LocalExposureGuidedUpsampling, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/ColorLUT.hsf", "ColorLUTCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::ColorLUT, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/ToneMapping.hsf", "ToneMappingCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::ToneMapping, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/PostProcessing/EditorSelectionOutlineMaskGen.hsf", "EditorSelectionOutlineMaskGenVS", "EditorSelectionOutlineMaskGenPS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::EditorSelectionOutlineMaskGen, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/EditorSelectionOutlineSetup.hsf", "EditorSelectionOutlineSetupCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::EditorSelectionOutlineSetup, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/EditorSelectionOutlineJumpFlood.hsf", "EditorSelectionOutlineJumpFloodCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::EditorSelectionOutlineJumpFlood, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/EditorSelectionOutlineComposite.hsf", "EditorSelectionOutlineCompositeCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::EditorSelectionOutlineComposite, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/VisualizePrimitiveID.hsf", "VisualizePrimitiveIDCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::VisualizePrimitiveID, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/VisualizeMaterialID.hsf", "VisualizeMaterialIDCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::VisualizeMaterialID, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/VisualizeWorldSpaceNormal.hsf", "VisualizeWorldSpaceNormalCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::VisualizeWorldSpaceNormal, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/VisualizeMotionVectors.hsf", "VisualizeMotionVectorsCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::VisualizeMotionVectors, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/VisualizeAmbientOcclusion.hsf", "VisualizeAmbientOcclusionCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::VisualizeAmbientOcclusion, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/VisualizeScreenSpaceShadowMask.hsf", "VisualizeScreenSpaceShadowMaskCS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::VisualizeScreenSpaceShadowMask, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/DebugDrawLines.hsf", "DebugDrawLinesVS", "DebugDrawLinesPS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::DebugDraw, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/GUIComposition.hsf", "GUICompositionVS", "GUICompositionPS");
        result |= shaderLibrary->LoadShader((uint32)RealTimeRendererShaderPiplineID::GUIComposition, shaderDesc);

        SkyAtmosphereConfig skyAtmosphereConfig;
        skyAtmosphere = CreateSkyAtmosphere(renderBackend, shaderLibrary, &skyAtmosphereConfig);

        #if 0
        if (renderEngine->IsHardwareRayTracingEnabled())
        {
            ShadingLanguage il = (GRenderBackend->GetType() == RenderBackendType::Vulkan) ? ShadingLanguage::SPIRV : ShadingLanguage::DXIL;

            std::vector<uint8> source;
            std::vector<std::wstring> includeDirs;
            std::vector<std::wstring> defines;
            includeDirs.push_back(HE_TEXT("../../../Shaders"));
            includeDirs.push_back(HE_TEXT("../../../Shaders/RealTimeRenderer"));
            defines.push_back(HE_TEXT("RAY_TRACING_ENABLED=1"));
            LoadShaderSourceFromFile("../../../Shaders/RealTimeRenderer/RayTracingShadows.hsf", source);

            RenderBackendRayTracingPipelineStateDesc rayTracingShadowsPipelineStateDesc = {
                .maxRayRecursionDepth = 1,
            };
            rayTracingShadowsPipelineStateDesc.shaders.push_back(RenderBackendRayTracingShaderDesc{
                .stage = RenderBackendShaderStage::RayGen,
                .entry = "RayTracingShadowsRayGen",
                });
            shaderLibrary->shaderCompiler->CompileShader_Depreacated(
                source,
                HE_TEXT("RayTracingShadowsRayGen"),
                RenderBackendShaderStage::RayGen,
                il,
                includeDirs,
                defines,
                &rayTracingShadowsPipelineStateDesc.shaders[0].code);

            rayTracingShadowsPipelineStateDesc.shaders.push_back(RenderBackendRayTracingShaderDesc{
                .stage = RenderBackendShaderStage::Miss,
                .entry = "RayTracingShadowsMiss",
                });
            shaderLibrary->shaderCompiler->CompileShader_Depreacated(
                source,
                HE_TEXT("RayTracingShadowsMiss"),
                RenderBackendShaderStage::Miss,
                il,
                includeDirs,
                defines,
                &rayTracingShadowsPipelineStateDesc.shaders[1].code);

            rayTracingShadowsPipelineStateDesc.shaderGroupDescs.resize(2);
            rayTracingShadowsPipelineStateDesc.shaderGroupDescs[0] = RenderBackendRayTracingShaderGroupDesc::CreateRayGen(0);
            rayTracingShadowsPipelineStateDesc.shaderGroupDescs[1] = RenderBackendRayTracingShaderGroupDesc::CreateMiss(1);

            rayTracingShadowsPipelineState = renderBackend->CreateRayTracingPipelineState(deviceMask, &rayTracingShadowsPipelineStateDesc, "rayTracingShadowsPipelineState");

            RenderBackendRayTracingShaderBindingTableDesc rayTracingShadowsSBTDesc = {
                .rayTracingPipelineState = rayTracingShadowsPipelineState,
                .numShaderRecords = 0,
            };
            rayTracingShadowsSBT = renderBackend->CreateRayTracingShaderBindingTable(deviceMask, &rayTracingShadowsSBTDesc, "rayTracingShadowsSBT");
        }
        #endif

        return result;
    }

    float RealTimeRenderer::GetHistoryExposureFromAutoExposureReadbackBuffer()
    {
        RenderBackendBufferHandle autoExposureReadbackBuffer = autoExposureReadbackBuffers[currentAutoExposureReadbackBufferIndex].buffer;
        if (autoExposureReadbackBuffer)
        {
            void* data = nullptr;
            renderBackend->MapBuffer(autoExposureReadbackBuffer, &data);
            float historyExposure = ((float*)data)[0];
            renderBackend->UnmapBuffer(autoExposureReadbackBuffer);
            return historyExposure;
        }
        return 1.0f;
    }

    void RealTimeRenderer::UpdatePerFrameData(const SceneView& view, RenderBackendCommandList* commandList)
    {
        uint32 deviceMask = ~0u;

        renderResolutionX = view.targetWidth;
        renderResolutionY = view.targetHeight;
        targetResolutionX = view.targetWidth;
        targetResolutionY = view.targetHeight;
        upscaleRatio = 1.0f;

        // Super Resolution
        isDLSSEnabled = false;
        isFSR2Enabled = false;
        isSuperResolutionEnabled = false;
        switch (settings.superResolutionTechnique)
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
            switch (settings.antialiasingTechnique)
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

        if (IsDLSSEnabled())
        {
            sl::Result slResult = sl::Result::eOk;
            sl::ViewportHandle slViewport = 0;

            sl::DLSSMode dlssMode = sl::DLSSMode::eOff;
            switch (settings.dlssSettings.qualityMode)
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
            dlssOptions.outputWidth = targetResolutionX;
            dlssOptions.outputHeight = targetResolutionY;
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

            renderResolutionX = dlssOptimalSettins.optimalRenderWidth;
            renderResolutionY = dlssOptimalSettins.optimalRenderHeight;

            upscaleRatio = (float)targetResolutionX / (float)renderResolutionX;

            // Dynamic Rendering
            // settings.minRenderSize.x = dlssOptimalSettins.renderWidthMin;
            // settings.minRenderSize.y = dlssOptimalSettins.renderHeightMin;
            // settings.maxRenderSize.x = dlssOptimalSettins.renderWidthMax;
            // settings.maxRenderSize.y = dlssOptimalSettins.renderHeightMax;

            LogInfo(GLogger, std::format("DLSS Mode: {}", (uint32)settings.dlssSettings.qualityMode));
            LogInfo(GLogger, std::format("TargetWidth {} TargetHeight {}", targetResolutionX, targetResolutionY));
            LogInfo(GLogger, std::format("RenderWidth {} RenderHeight {}", renderResolutionX, renderResolutionY));
            LogInfo(GLogger, std::format("DynamicMinimumRenderSizeWidth {} DynamicMinimumRenderSizeHeight {}", dlssOptimalSettins.renderWidthMin, dlssOptimalSettins.renderHeightMin));
            LogInfo(GLogger, std::format("DynamicMaximumRenderSizeWidth {} DynamicMaximumRenderSizeHeight {}", dlssOptimalSettins.renderWidthMax, dlssOptimalSettins.renderHeightMax));
        }
        else if (IsFSR2Enabled())
        {
            if (settings.fsr2Settings.qualityMode == FSR2QualityMode::Custom)
            {
                upscaleRatio = settings.fsr2Settings.customUpscaleRatio;

                renderResolutionX = (uint32)((float)targetResolutionX / upscaleRatio);
                renderResolutionY = (uint32)((float)targetResolutionY / upscaleRatio);
            }
            else
            {
                FfxFsr2QualityMode fsr2QualityMode = (FfxFsr2QualityMode)(int)settings.fsr2Settings.qualityMode;
                FfxErrorCode errorCode = ffxFsr2GetRenderResolutionFromQualityMode(
                    &renderResolutionX,
                    &renderResolutionY,
                    targetResolutionX,
                    targetResolutionY,
                    fsr2QualityMode);
                FFX_ASSERT(errorCode == FFX_OK);

                upscaleRatio = ffxFsr2GetUpscaleRatioFromQualityMode(fsr2QualityMode);
            }
        }

        float upscaleRatio = 1.0f;

        auto nonJitteredPrevProjectionMatrix = sceneViewShaderParameters.nonJitteredProjectionMatrix;
        auto nonJitteredPrevViewProjectionMatrix = sceneViewShaderParameters.nonJitteredViewProjectionMatrix;
        auto nonJitteredPrevInvViewProjectionMatrix = sceneViewShaderParameters.nonJitteredInvViewProjectionMatrix;

        auto jitteredPrevProjectionMatrix = sceneViewShaderParameters.projectionMatrix;
        auto jitteredPrevViewProjectionMatrix = sceneViewShaderParameters.viewProjectionMatrix;
        auto jitteredPrevInvViewProjectionMatrix = sceneViewShaderParameters.invViewProjectionMatrix;

        auto jitteredProjectionMatrix = view.camera.projectionMatrix;

        Vector2 cameraJitterOffset = { 0.0f, 0.0f };
        if (ShouldApplyCameraJittering())
        {
            if (IsFSR2Enabled())
            {
                float jitterX = 0;
                float jitterY = 0;
                const int32 jitterPhaseCount = ffxFsr2GetJitterPhaseCount(renderResolutionX, targetResolutionX);
                FfxErrorCode errorCode = ffxFsr2GetJitterOffset(&jitterX, &jitterY, sceneViewShaderParameters.frameIndex, jitterPhaseCount);
                FFX_ASSERT(errorCode == FFX_OK);

                Vector2 jitterOffset = { jitterX * 2.0f / (float)renderResolutionX, -jitterY * 2.0f / (float)renderResolutionY };

                jitteredProjectionMatrix[2][0] += -jitterOffset.x;
                jitteredProjectionMatrix[2][1] += -jitterOffset.y;

                cameraJitterOffset.x = jitterX;
                cameraJitterOffset.y = jitterY;
            }
            else
            {
                JitterProjectionMatrix(jitteredProjectionMatrix, cameraJitterOffset, { renderResolutionX, renderResolutionY }, upscaleRatio);
            }
        }
        auto jitteredInvProjectionMatrix = Math::Inverse(jitteredProjectionMatrix);

        Vector2 prevCameraJitterOffset = sceneViewShaderParameters.cameraJitterOffset;
        if (view.frameIndex == 0)
        {
            prevCameraJitterOffset = cameraJitterOffset;
        }

        Vector3 prevCameraPosition = sceneViewShaderParameters.cameraPosition;

        float prevSceneColorPreExposure = sceneViewShaderParameters.sceneColorPreExposure;
        if (view.frameIndex == 0)
        {
            prevSceneColorPreExposure = 1.0f;
        }

        Scene* scene = view.scene;
        RenderSystem* renderer = (RenderSystem*)view.renderEngine;

        SkyAtmosphereComponent* skyAtmosphere = renderer->skyAtmosphereComponent;
        LightComponent* sunLight = renderer->skyAtmosphereLight;

        float bottomRadius = skyAtmosphere->groundRadius;
        Vector3 planetCenterKm = Vector3(0.0f, 0.0f, -bottomRadius);
        Vector3 worldSpacePlanetCenter = planetCenterKm * KM_TO_M;

        Vector3 cameraForward = view.camera.forwardVec;
        Vector3 worldSpaceCameraPosition = view.camera.position;

        if (sunLight)
        {
            const float sunLightHalfApexAngleRadian = sunLight->GetSunLightHalfApexAngleRadian();
            const float atmosphereLightDiscCosHalfApexAngle = std::cos(sunLightHalfApexAngleRadian);
            const float sunSolidAngle = 2.0f * M_PI * (1.0f - atmosphereLightDiscCosHalfApexAngle); // Solid angle from aperture https://en.wikipedia.org/wiki/Solid_angle
            const Vector3 sunOuterSpaceIlluminance = sunLight->GetPhysicalLightColor();
            const Vector3 atmosphereLightDiscLuminance = sunLight->atmosphereLightDiskColorTint * sunOuterSpaceIlluminance / sunSolidAngle; // approximation

            sceneViewShaderParameters.atmosphereLightDirection = -sunLight->GetDirection();
            sceneViewShaderParameters.atmosphereLightDiscLuminance = atmosphereLightDiscLuminance;
            sceneViewShaderParameters.atmosphereLightDiscCosHalfApexAngle = atmosphereLightDiscCosHalfApexAngle;

            // Compute the referential for the sky view LUT
            Matrix3x3 skyAtmosphereSkyViewLUTReferential;
            Vector3 planetCenterToCameraKm = (worldSpaceCameraPosition - worldSpacePlanetCenter) * M_TO_KM;
            Vector3 upVector = Math::Normalize(planetCenterToCameraKm);
            Vector3 forwardVector = cameraForward;
            if (std::abs(Math::DotProduct(upVector, forwardVector)) > 0.999f)
            {
                // When it becomes hard to generate a referential, generate it procedurally.
                // [ Duff et al. 2017, "Building an Orthonormal Basis, Revisited" ]
                const float sign = upVector.z >= 0.0f ? 1.0f : -1.0f;
                const float a = -1.0f / (sign + upVector.z);
                const float b = upVector.x * upVector.y * a;
                forwardVector = Vector3(1 + sign * a * std::pow(upVector.x, 2.0f), sign * b, -sign * upVector.x);
                Vector3 leftVector = Vector3(b, sign + a * std::pow(upVector.y, 2.0f), -upVector.y);
                skyAtmosphereSkyViewLUTReferential = Matrix3x3(forwardVector, leftVector, upVector);
            }
            else
            {
                // This is better as it should be more stable with respect to camera forward.
                Vector3    leftVector = Math::Normalize(Math::CrossProduct(forwardVector, upVector));
                forwardVector = Math::Normalize(Math::CrossProduct(upVector, leftVector));
                skyAtmosphereSkyViewLUTReferential = Matrix3x3(forwardVector, leftVector, upVector);
            }

            sceneViewShaderParameters.skyAtmosphereSkyViewLUTReferential = skyAtmosphereSkyViewLUTReferential;
            sceneViewShaderParameters.skyAtmosphereLightOuterSpaceIlluminance = sunOuterSpaceIlluminance;
        }
        else
        {
            sceneViewShaderParameters.atmosphereLightDirection = Vector3(0.0f, 0.0f, 1.0f);
            sceneViewShaderParameters.atmosphereLightDiscLuminance = Vector3(0.0f, 0.0f, 0.0f);
            sceneViewShaderParameters.atmosphereLightDiscCosHalfApexAngle = 0.0f;
            sceneViewShaderParameters.skyAtmosphereSkyViewLUTReferential = Matrix3x3(1.0f);
            sceneViewShaderParameters.skyAtmosphereLightOuterSpaceIlluminance = { 0.0f, 0.0f, 0.0f };
        }

        float sceneColorPreExposure = 1.0f;
        if (view.frameIndex != 0)
        {
            sceneColorPreExposure = GetHistoryExposureFromAutoExposureReadbackBuffer();
        }
        if (settings.fixedPreExposureEnabled)
        {
            sceneColorPreExposure = settings.fixedPreExposure;
        }

        sceneViewShaderParameters.frameIndex = view.frameIndex;
        sceneViewShaderParameters.blueNoisePhase = (view.frameIndex & 0xFF) * 1.6180339887f;
        sceneViewShaderParameters.deltaTime = view.deltaTime;
        sceneViewShaderParameters.gamma = 2.2f;
        sceneViewShaderParameters.exposure = settings.postProcessingSettings.autoExposureExposureCompensation;
        if (renderResolutionX != targetResolutionX || renderResolutionY != targetResolutionY)
        {
            sceneViewShaderParameters.mipLodBias = std::log2f((float)renderResolutionX / (float)targetResolutionX);
        }
        else
        {
            sceneViewShaderParameters.mipLodBias = 0.0f;
        }
        sceneViewShaderParameters.sceneColorPreExposure = sceneColorPreExposure;
        sceneViewShaderParameters.sceneColorOneOverPreExposure = 1.0f / sceneColorPreExposure;
        sceneViewShaderParameters.historySceneColorPreExposureCorrection = sceneColorPreExposure / prevSceneColorPreExposure;
        sceneViewShaderParameters.skyAtmosphereBottomRadiusInKilometers = skyAtmosphere->groundRadius;
        sceneViewShaderParameters.skyAtmosphereTopRadiusInKilometers = skyAtmosphere->groundRadius + skyAtmosphere->atmosphereHeight;
        sceneViewShaderParameters.skyAtmosphereSkyLuminanceFactor = skyAtmosphere->skyLuminanceTint * skyAtmosphere->skyLuminanceIntensity;
        sceneViewShaderParameters.indirectLightingFactor = settings.indirectLightingTint * settings.indirectLightingIntensity;
        sceneViewShaderParameters.cameraPosition = view.camera.position;
        sceneViewShaderParameters.prevCameraPosition = prevCameraPosition;
        sceneViewShaderParameters.cameraJitterOffset = cameraJitterOffset;
        sceneViewShaderParameters.prevCameraJitterOffset = prevCameraJitterOffset;
        sceneViewShaderParameters.cameraUp = view.camera.upVec;
        sceneViewShaderParameters.cameraRight = view.camera.rightVec;
        sceneViewShaderParameters.cameraForward = view.camera.forwardVec;
        sceneViewShaderParameters.cameraNearPlane = view.camera.nearClippingPlane;
        sceneViewShaderParameters.cameraFarPlane = view.camera.farClippingPlane;
        sceneViewShaderParameters.cameraHalfFovRad = Math::DegreesToRadians(view.camera.fieldOfView) * 0.5f;
        sceneViewShaderParameters.cameraAspectRatio = view.camera.aspectRatio;
        sceneViewShaderParameters.viewMatrix = view.camera.viewMatrix;
        sceneViewShaderParameters.invViewMatrix = view.camera.invViewMatrix;
        sceneViewShaderParameters.projectionMatrix = jitteredProjectionMatrix;
        sceneViewShaderParameters.inverseProjectionMatrix = jitteredInvProjectionMatrix;
        sceneViewShaderParameters.viewProjectionMatrix = jitteredProjectionMatrix * view.camera.viewMatrix;
        sceneViewShaderParameters.invViewProjectionMatrix = view.camera.invViewMatrix * jitteredInvProjectionMatrix;
        sceneViewShaderParameters.prevProjectionMatrix = jitteredPrevProjectionMatrix;
        sceneViewShaderParameters.prevViewProjectionMatrix = jitteredPrevViewProjectionMatrix;
        sceneViewShaderParameters.prevInvViewProjectionMatrix = jitteredPrevInvViewProjectionMatrix;
        sceneViewShaderParameters.nonJitteredProjectionMatrix = view.camera.projectionMatrix;
        sceneViewShaderParameters.nonJitteredInvProjectionMatrix = view.camera.invProjectionMatrix;
        sceneViewShaderParameters.nonJitteredViewProjectionMatrix = view.camera.projectionMatrix * view.camera.viewMatrix;
        sceneViewShaderParameters.nonJitteredInvViewProjectionMatrix = view.camera.invViewMatrix * view.camera.invProjectionMatrix;
        sceneViewShaderParameters.nonJitteredPrevProjectionMatrix = nonJitteredPrevProjectionMatrix;
        sceneViewShaderParameters.nonJitteredPrevViewProjectionMatrix = nonJitteredPrevViewProjectionMatrix;
        sceneViewShaderParameters.nonJitteredPrevInvViewProjectionMatrix = nonJitteredPrevInvViewProjectionMatrix;
        sceneViewShaderParameters.renderResolutionX = renderResolutionX;
        sceneViewShaderParameters.renderResolutionY = renderResolutionY;
        sceneViewShaderParameters.targetResolutionX = targetResolutionX;
        sceneViewShaderParameters.targetResolutionY = targetResolutionY;
        sceneViewShaderParameters.renderResolutionAndInvRenderResolution = Vector4(1.0f * renderResolutionX, 1.0f * renderResolutionY, 1.0f / renderResolutionX, 1.0f / renderResolutionY);
        sceneViewShaderParameters.targetResolutionAndInvTargetResolution = Vector4(1.0f * targetResolutionX, 1.0f * targetResolutionY, 1.0f / targetResolutionX, 1.0f / targetResolutionY);

        float autoExposureMinExposureValue = settings.postProcessingSettings.autoExposureMinExposureValue;
        float autoExposureMaxExposureValue = settings.postProcessingSettings.autoExposureMaxExposureValue;

        if (settings.exposureMethod == ExposureMethod::FixedExposure)
        {
            autoExposureMinExposureValue = autoExposureMaxExposureValue = settings.postProcessingSettings.fixedExposureValue;
        }

        sceneViewShaderParameters.autoExposureExposureCompensation = settings.postProcessingSettings.autoExposureExposureCompensation;
        sceneViewShaderParameters.autoExposureMinExposureValue = autoExposureMinExposureValue;
        sceneViewShaderParameters.autoExposureMaxExposureValue = autoExposureMaxExposureValue;
        sceneViewShaderParameters.autoExposureHistogramLowPercent = settings.postProcessingSettings.autoExposureHistogramLowPercent;
        sceneViewShaderParameters.autoExposureHistogramHighPercent = settings.postProcessingSettings.autoExposureHistogramHighPercent;
        sceneViewShaderParameters.autoExposureHistogramMinEV100 = settings.postProcessingSettings.autoExposureHistogramMinEV100;
        sceneViewShaderParameters.autoExposureHistogramMaxEV100 = settings.postProcessingSettings.autoExposureHistogramMaxEV100;
        sceneViewShaderParameters.autoExposureSpeedDarkToBright = settings.postProcessingSettings.autoExposureSpeedDarkToBright;
        sceneViewShaderParameters.autoExposureSpeedBrightToDark = settings.postProcessingSettings.autoExposureSpeedBrightToDark;
        sceneViewShaderParameters.autoExposureUseTargetExposure = 0;
        sceneViewShaderParameters.bloomIntensity = settings.postProcessingSettings.bloomIntensity;
        sceneViewShaderParameters.bloomRadius = settings.postProcessingSettings.bloomRadius;
        sceneViewShaderParameters.lensDirtIntensity = settings.postProcessingSettings.lensDirtIntensity;
        sceneViewShaderParameters.lensDirtTint = settings.postProcessingSettings.lensDirtTint;
        sceneViewShaderParameters.lensFlaresIntensity = settings.postProcessingSettings.lensFlaresIntensity;
        sceneViewShaderParameters.chromaticAberrationIntensity = settings.postProcessingSettings.chromaticAberrationIntensity;
        sceneViewShaderParameters.chromaticAberrationOffset = settings.postProcessingSettings.chromaticAberrationOffset;
        sceneViewShaderParameters.colorGradingWhiteBalanceColorTemperature = settings.postProcessingSettings.colorGradingWhiteBalanceColorTemperature;
        sceneViewShaderParameters.colorCorrectionSaturation = settings.postProcessingSettings.colorCorrectionSaturation;
        sceneViewShaderParameters.colorCorrectionContrast = settings.postProcessingSettings.colorCorrectionContrast;
        sceneViewShaderParameters.colorCorrectionGamma = settings.postProcessingSettings.colorCorrectionGamma;
        sceneViewShaderParameters.colorCorrectionGain = settings.postProcessingSettings.colorCorrectionGain;
        sceneViewShaderParameters.colorCorrectionOffset = settings.postProcessingSettings.colorCorrectionOffset;

        renderBackend->UpdateBuffer(sceneViewShaderParametersUploadBuffer, 0, &sceneViewShaderParameters, sizeof(RealTimeRendererSceneViewShaderParameters));
        commandList->CopyBuffer(
            sceneViewShaderParametersUploadBuffer,
            0,
            sceneViewShaderParametersBuffer,
            0,
            sizeof(RealTimeRendererSceneViewShaderParameters));

        isSurfelGIEnabled = false;
        isRayTracingShadowsEnabled = settings.shadowsTechnique == ShadowsTechnique::RayTracingShadows;
        isRayTracingReflectionsEnabled = settings.reflectionsTechnique == ReflectionsTechnique::RayTracingReflections;
        isRayTracingAmbientOcclusionEnabled = settings.ambientOcclusionTechnique == AmbientOcclusionTechnique::RayTracingAmbientOcclusion;

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
#else
        bool hardwareRayTracingSupport = GRenderer->IsHardwareRayTracingEnabled();
        if (hardwareRayTracingSupport)
        {
            GRenderer->SetShouldUpdateRayTracingScene(true);
        }
#endif

        if (!historyAutoExposureBufferPersistent.IsValid())
        {
            historyAutoExposureBufferPersistent.active = true;
            historyAutoExposureBufferPersistent.name = "HistoryAutoExposureBuffer";
            historyAutoExposureBufferPersistent.desc = RenderGraphBufferDesc::CreateByteAddress(16);
            historyAutoExposureBufferPersistent.buffer = renderBackend->CreateBuffer(deviceMask, &historyAutoExposureBufferPersistent.desc, nullptr, historyAutoExposureBufferPersistent.name.c_str());
            historyAutoExposureBufferPersistent.initialState = RenderBackendResourceState::UnorderedAccess;
        }
    }

    bool GatherRayTracingInstances(
        RenderGraph& renderGraph,
        SceneView& view,
        RayTracingScene& rayTracingScene)
    {
        return false;
    }

    RenderGraphTextureHandle RealTimeRenderer::RenderUIColorAndAlpha(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        auto uiColorAndAlphaTexture = sceneTextures.uiColorAndAlphaTexture;

        renderGraph.AddPass(std::format("UIColorAndAlpha (Graphics, {}x{})", targetResolutionX, targetResolutionY), RenderGraphPassFlags::Graphics | RenderGraphPassFlags::SkipRenderPass,
            [&](RenderGraphBuilder& builder)
            {
                uiColorAndAlphaTexture = sceneTextures.uiColorAndAlphaTexture = builder.WriteTexture(sceneTextures.uiColorAndAlphaTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, uiColorAndAlphaTexture, RenderBackendRenderTargetLoadOp::Clear, RenderBackendRenderTargetStoreOp::Store);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    renderEngine->DrawUI(commandList, registry.GetRenderBackendTexture(uiColorAndAlphaTexture));
                };
            });

        return uiColorAndAlphaTexture;
    }

    void RealTimeRenderer::SetupRenderGraph(RenderGraph& renderGraph, const SceneView& view)
    {
        OPTICK_EVENT();

        uint32 deviceMask = ~0u;

        // TODO: move to other place
        shaderLibrary->HotReload();

        auto& sceneTextures = renderGraph.blackboard.CreateSingleton<RealTimeRendererSceneTextures>();
        auto& historyInfo = renderGraph.blackboard.CreateSingleton<RealTimeRendererHistoryInfo>();
        auto& finalTextureData = renderGraph.blackboard.CreateSingleton<RenderGraphFinalTexture>();
        auto& ouptutTextureData = renderGraph.blackboard.CreateSingleton<RenderGraphOutputTexture>();

        auto& debugViewModeTextures = renderGraph.blackboard.CreateSingleton<RealTimeRendererDebugViewModeTextures>();

        RenderBackendTextureClearValue clearColor = RenderBackendTextureClearValue::CreateColorValueFloat4(0.0f, 0.0f, 0.0f, 0.0f);
        RenderBackendTextureClearValue clearDepth = RenderBackendTextureClearValue::CreateDepthValue(FarClipPlaneDepthValue);
        RenderBackendTextureClearValue clearVisibilityBufferColor = RenderBackendTextureClearValue::CreateColorValueUnit4(0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF);

        const bool shouldRenderSkyAtmosphere = ShouldRenderSkyAtmosphere();
        const bool shouldRenderAmbientOcclusion = settings.ambientOcclusionTechnique != AmbientOcclusionTechnique::None;

        if (IsDLAAEnabled())
        {
            sl::Result slResult = sl::Result::eOk;
            sl::ViewportHandle slViewport = 0;

            sl::DLSSOptions dlssOptions = {};
            dlssOptions.mode = sl::DLSSMode::eMaxQuality;
            dlssOptions.outputWidth = targetResolutionX;
            dlssOptions.outputHeight = targetResolutionY;
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
            Matrix4x4 reprojectionMatrix = sceneViewShaderParameters.nonJitteredInvViewProjectionMatrix * sceneViewShaderParameters.nonJitteredPrevViewProjectionMatrix;
            Matrix4x4 inverseReprojectionMatrix = glm::inverse(reprojectionMatrix);

            sl::Constants constants = {};
            constants.cameraViewToClip = *((sl::float4x4*)&sceneViewShaderParameters.nonJitteredProjectionMatrix);
            constants.clipToCameraView = *((sl::float4x4*)&sceneViewShaderParameters.nonJitteredInvProjectionMatrix);
            //constants.clipToLensClip = {};
            constants.clipToPrevClip = *((sl::float4x4*)&reprojectionMatrix);
            constants.prevClipToClip = *((sl::float4x4*)&inverseReprojectionMatrix);
            constants.jitterOffset = sl::float2(sceneViewShaderParameters.cameraJitterOffset.x, sceneViewShaderParameters.cameraJitterOffset.y);
            constants.mvecScale = { 1.0f, 1.0f };//sl::float2(perFrameData.data.renderResolutionX, perFrameData.data.renderResolutionY);
            constants.cameraPinholeOffset = { 0.0f, 0.0f };
            constants.cameraPos = sl::float3(sceneViewShaderParameters.cameraPosition.x, sceneViewShaderParameters.cameraPosition.y, sceneViewShaderParameters.cameraPosition.z);
            constants.cameraUp = sl::float3(sceneViewShaderParameters.cameraUp.x, sceneViewShaderParameters.cameraUp.y, sceneViewShaderParameters.cameraUp.z);
            constants.cameraRight = sl::float3(sceneViewShaderParameters.cameraRight.x, sceneViewShaderParameters.cameraRight.y, sceneViewShaderParameters.cameraRight.z);
            constants.cameraFwd = sl::float3(sceneViewShaderParameters.cameraForward.x, sceneViewShaderParameters.cameraForward.y, sceneViewShaderParameters.cameraForward.z);
            constants.cameraNear = sceneViewShaderParameters.cameraNearPlane;
            constants.cameraFar = sceneViewShaderParameters.cameraFarPlane;
            constants.cameraFOV = sceneViewShaderParameters.cameraHalfFovRad * 2.0f;
            constants.cameraAspectRatio = sceneViewShaderParameters.cameraAspectRatio;
            constants.motionVectorsInvalidValue = sl::INVALID_FLOAT;
            constants.depthInverted = sl::Boolean::eTrue;
            constants.cameraMotionIncluded = sl::Boolean::eTrue;
            constants.motionVectors3D = sl::Boolean::eFalse;
            constants.reset = sceneViewShaderParameters.frameIndex == 0 ? sl::Boolean::eTrue : sl::Boolean::eFalse;
            constants.orthographicProjection = sl::Boolean::eFalse;
            constants.motionVectorsDilated = sl::Boolean::eFalse;
            constants.motionVectorsJittered = sl::Boolean::eFalse;

            sl::Result result = sl::Result::eOk;
            sl::ViewportHandle viewport = 0;
            sl::FrameToken* frameToken = nullptr;
            if (SL_FAILED(result, slGetNewFrameToken(frameToken, &sceneViewShaderParameters.frameIndex)))
            {
                LogError(GLogger, std::format("slGetNewFrameToken, error code: {}", (int32)result));
            }
            if (SL_FAILED(result, slSetConstants(constants, *frameToken, viewport)))
            {
                LogError(GLogger, std::format("slSetConstants, error code: {}", (int32)result));
            }
        }

        RenderGraphTextureDesc vbuffer0Desc = RenderGraphTextureDesc::Create2D(
            renderResolutionX,
            renderResolutionY,
            RenderBackendTextureFormat::RG32Uint,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget,
            clearVisibilityBufferColor);
        sceneTextures.vbuffer0 = renderGraph.CreateTexture(vbuffer0Desc, "VBuffer0");

        RenderGraphTextureDesc vbuffer1Desc = RenderGraphTextureDesc::Create2D(
            renderResolutionX,
            renderResolutionY,
            RenderBackendTextureFormat::RGBA32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget,
            clearVisibilityBufferColor);
        sceneTextures.vbuffer1 = renderGraph.CreateTexture(vbuffer1Desc, "VBuffer1");

        RenderGraphTextureDesc gbuffer0Desc = RenderGraphTextureDesc::Create2D(
            renderResolutionX,
            renderResolutionY,
            RenderBackendTextureFormat::RGB10A2Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            clearColor);
        sceneTextures.gbuffer0 = renderGraph.CreateTexture(gbuffer0Desc, "GBuffer0");

        RenderGraphTextureDesc gbuffer1Desc = RenderGraphTextureDesc::Create2D(
            renderResolutionX,
            renderResolutionY,
            RenderBackendTextureFormat::RGBA8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            clearColor);
        sceneTextures.gbuffer1 = renderGraph.CreateTexture(gbuffer1Desc, "GBuffer1");

        RenderGraphTextureDesc gbuffer2Desc = RenderGraphTextureDesc::Create2D(
            renderResolutionX,
            renderResolutionY,
            RenderBackendTextureFormat::RGBA8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            clearColor);
        sceneTextures.gbuffer2 = renderGraph.CreateTexture(gbuffer2Desc, "GBuffer2");

        RenderGraphTextureDesc sceneColorTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolutionX,
            renderResolutionY,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget,
            RenderBackendTextureClearValue::None,
            1,
            1,
            RenderBackendResourceState::UnorderedAccess);
        sceneTextures.sceneColorTextureDesc = sceneColorTextureDesc;
        sceneTextures.sceneColorTexture = renderGraph.CreateTexture(sceneColorTextureDesc, "SceneColorTexture");

        RenderGraphTextureDesc sceneDepthTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolutionX,
            renderResolutionY,
            RenderBackendTextureFormat::D32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::DepthStencil,
            clearDepth,
            1,
            1,
            RenderBackendResourceState::DepthStencil);
        sceneTextures.sceneDepthTexture = renderGraph.CreateTexture(sceneDepthTextureDesc, "SceneDepthTexture");

        RenderGraphTextureDesc motionVectorTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolutionX,
            renderResolutionY,
            RenderBackendTextureFormat::RG16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        sceneTextures.motionVectorTexture = renderGraph.CreateTexture(motionVectorTextureDesc, "MotionVectorTexture");

        RenderGraphTextureDesc uiColorAndAlphaTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolutionX,
            targetResolutionY,
            RenderBackendTextureFormat::RGB10A2Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget);
        sceneTextures.uiColorAndAlphaTexture = renderGraph.CreateTexture(uiColorAndAlphaTextureDesc, "UIColorAndAlphaTexture");

        RenderGraphTextureDesc finalTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolutionX,
            targetResolutionY,
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
        uint32 hzbWidth = Math::Max(Math::RoundUpToPowerOfTwo(renderResolutionX) >> 1, 1u);
        uint32 hzbHeight = Math::Max(Math::RoundUpToPowerOfTwo(renderResolutionY) >> 1, 1u);
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
            RenderSkyAtmosphereLUTs(renderGraph, *skyAtmosphere, *(renderEngine->skyAtmosphereComponent), renderResolutionX, renderResolutionY, sceneViewShaderParametersBuffer);
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
            renderResolutionX,
            renderResolutionY,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle reflectionsTexture = renderGraph.CreateTexture(reflectionsTextureDesc, "ReflectionsTexture");

        RenderGraphTextureHandle ssrDebugOutputTexture = renderGraph.CreateTexture(RenderGraphTextureDesc::Create2D(
            renderResolutionX,
            renderResolutionY,
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
                            RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(reflectionsTexture), 0),
                            RenderBackendTextureClearValue::Black);
                    };
                });
        }

        AddIndirectLightingSpecularPass(renderGraph, view);

        // AddSurfleGIPasses(renderGraph, view);

        RenderGraphTextureDesc screenSpaceShadowMaskTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolutionX,
            renderResolutionY,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture");

        if (view.debugViewMode == DebugViewMode::ShadowMask)
        {
            debugViewModeTextures.screenSpaceShadowMaskTextureDesc = screenSpaceShadowMaskTextureDesc;
            debugViewModeTextures.screenSpaceShadowMaskTexture = renderGraph.CreateTexture(screenSpaceShadowMaskTextureDesc, "ScreenSpaceShadowMaskTexture (Copy)");
        }

        RenderGraphTextureDesc rayDistanceDesc = RenderGraphTextureDesc::Create2D(
            renderResolutionX,
            renderResolutionY,
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
                if (view.debugViewMode == DebugViewMode::ShadowMask)
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
                                        registry.GetRenderBackendTexture(screenSpaceShadowMaskTexture),
                                        offset,
                                        0,
                                        registry.GetRenderBackendTexture(screenSpaceShadowMaskTextureCopy),
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
        //            uint32 dispatchX = Math::CeilDiv(renderResolutionX, 8);
        //            uint32 dispatchY = Math::CeilDiv(renderResolutionY, 8);

        //            RenderBackendGraphicsPipelineState graphicsPipelineState = {};
        //            graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
        //            graphicsPipelineState.depthStencilState.depthTestEnable = true;
        //            graphicsPipelineState.depthStencilState.depthWriteEnable = false;
        //            graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::Equal;

        //            RenderBackendShaderArguments shaderArguments = {};
        //            shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
        //            shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(renderEngine->environmentMap));
        //            shaderArguments.PushConstants(0, 0.0f);

        //            RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SkyBox);
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

        if (shouldRenderSubsurfaceScattering)
        {
            RenderSubsurfaceScattering(renderGraph, view);
        }

        if (shouldRenderSkyAtmosphere)
        {
            RenderSkyAtmosphere(renderGraph, *skyAtmosphere, *(renderEngine->skyAtmosphereComponent), renderResolutionX, renderResolutionY, sceneViewShaderParametersBuffer);
        }

        const bool isVisualizeSufelEnabled = (view.debugViewMode == DebugViewMode::SurfelGISurfel);

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

        renderGraph.AddPass(std::format("FinalComposite (Graphics, {}x{})", targetResolutionX, targetResolutionY), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                auto& finalColorTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();

                auto uiColorAndAlphaTexture = builder.ReadTexture(sceneTextures.uiColorAndAlphaTexture, RenderBackendResourceState::ShaderResource);
                auto finalColorTexture = finalColorTextureData.finalTexture = builder.WriteTexture(finalColorTextureData.finalTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, finalColorTexture, RenderBackendRenderTargetLoadOp::Load, RenderBackendRenderTargetStoreOp::Store);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, (float)targetResolutionX, (float)targetResolutionY);
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, targetResolutionX, targetResolutionY);
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
                    shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(uiColorAndAlphaTexture)));

                    RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::GUIComposition);
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
                    Extent2D extent = { targetResolutionX, targetResolutionY };

                    commandList.CopyTexture2D(
                        registry.GetRenderBackendTexture(finalTexture),
                        offset,
                        0,
                        registry.GetRenderBackendTexture(outputTexture),
                        offset,
                        0,
                        extent);

                    RenderBackendBarrier transitions[] =
                    {
                        RenderBackendBarrier(registry.GetRenderBackendTexture(outputTexture), RenderBackendTextureSubresourceRange(0, 1, 0, 1), RenderBackendResourceState::CopyDst, RenderBackendResourceState::Present)
                    };
                    commandList.Transitions(transitions, 1);
                };
            });
    }
}