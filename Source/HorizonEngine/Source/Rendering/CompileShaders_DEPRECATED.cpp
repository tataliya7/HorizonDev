#include "ShaderID_DEPRECATED.h"

namespace Horizon
{
    static void CompileSkyAtmosphereShaders_DEPRECATED(ShaderLibrary_DEPRECATED* shaderLibrary)
    {
        ShaderDesc shaderDesc;
        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SkyAtmosphereTransmittanceLut.hsf", "SkyAtmosphereTransmittanceLutCS");
        shaderDesc.AddDefine("SKY_ATMOSPHERE_TRANSMITTANCE_LUT_PASS", 1);
        shaderLibrary->LoadShader(ShaderID::SkyAtmosphereTransmittanceLut, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SkyAtmosphereMultipleScatteringLut.hsf", "SkyAtmosphereMultipleScatteringLutCS");
        shaderDesc.AddDefine("SKY_ATMOSPHERE_MULTIPLE_SCATTERING_LUT_PASS", 1);
        shaderLibrary->LoadShader(ShaderID::SkyAtmosphereMultipleScatteringLut, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SkyAtmosphereSkyViewLut.hsf", "SkyAtmosphereSkyViewLutCS");
        shaderDesc.AddDefine("SKY_ATMOSPHERE_SKY_VIEW_LUT_PASS", 1);
        shaderLibrary->LoadShader(ShaderID::SkyAtmosphereSkyViewLut, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SkyAtmosphereAerialPerspectiveVolume.hsf", "SkyAtmosphereAerialPerspectiveVolumeCS");
        shaderDesc.AddDefine("SKY_ATMOSPHERE_AERIAL_PERSPECTIVE_VOLUME_PASS", 1);
        shaderLibrary->LoadShader(ShaderID::SkyAtmosphereAerialPerspectiveVolume, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/SkyAtmosphereRayMarching.hsf", "FullScreenQuadVS", "SkyAtmosphereRayMarchingPS");
        //shaderDesc = ShaderDesc::CreateGraphics("FullScreenQuadVertexShader.hsf", "FullScreenQuadVS", "RealTimeRenderer/SkyAtmosphereRayMarching.hsf", "SkyAtmosphereRayMarchingPS");
        shaderDesc.AddDefine("SKY_ATMOSPHERE_RAY_MARCHING_PASS", 1);
        shaderLibrary->LoadShader(ShaderID::SkyAtmosphereRayMarching, shaderDesc);
    }

    static void CompileRayTracingShaders_DEPRECATED(ShaderLibrary_DEPRECATED* shaderLibrary)
    {
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

            rayTracingShadowsPipelineState = renderBackend->CreateRayTracingPipelineState(&rayTracingShadowsPipelineStateDesc, "rayTracingShadowsPipelineState");

            RenderBackendRayTracingShaderBindingTableDesc rayTracingShadowsSBTDesc = {
                .rayTracingPipelineState = rayTracingShadowsPipelineState,
                .numShaderRecords = 0,
            };
            rayTracingShadowsSBT = renderBackend->CreateRayTracingShaderBindingTable(&rayTracingShadowsSBTDesc, "rayTracingShadowsSBT");
        }
#endif
    }

    void CompileShaders_DEPRECATED(ShaderLibrary_DEPRECATED* shaderLibrary)
    {
        CompileSkyAtmosphereShaders_DEPRECATED(shaderLibrary);
        CompileRayTracingShaders_DEPRECATED(shaderLibrary);

        ShaderDesc shaderDesc;

        shaderDesc = ShaderDesc::CreateCompute("PreIntegratedBRDF.hsf", "PreIntegratedBRDFCS");
        shaderLibrary->LoadShader(ShaderID::PreIntegratedBRDF, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("EquirectangularToCubemap.hsf", "EquirectangularToCubemapCS");
        shaderLibrary->LoadShader(ShaderID::EquirectangularToCubemap, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("DownsampleCubemap.hsf", "DownsampleCubemapCS");
        shaderLibrary->LoadShader(ShaderID::DownsampleCubemap, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("DownsampleTexture2D.hsf", "DownsampleTexture2DCS");
        shaderLibrary->LoadShader(ShaderID::DownsampleTexture2D, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("DownsampleTexture2D_PS.hsf", "DownsampleTexture2D_VS", "DownsampleTexture2D_PS");
        shaderLibrary->LoadShader(ShaderID::DownsampleTexture2D_PS, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("ComputeEnvironmentIrradiance.hsf", "ComputeEnvironmentIrradianceCS");
        shaderLibrary->LoadShader(ShaderID::ComputeEnvironmentIrradiance, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("ComputeEnvironmentIrradianceSH.hsf", "ComputeEnvironmentIrradianceSHCS");
        shaderLibrary->LoadShader(ShaderID::ComputeEnvironmentIrradianceSH, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("FilterEnvironmentMap.hsf", "FilterEnvironmentMapCS");
        shaderLibrary->LoadShader(ShaderID::FilterEnvironmentMap, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("GPUFFT.hsf", "SharedMemoryComplexFFTCS");
        shaderLibrary->LoadShader(ShaderID::SharedMemoryComplexFFT, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("GPUFFT.hsf", "SharedMemoryComplexIFFTCS");
        shaderLibrary->LoadShader(ShaderID::SharedMemoryComplexIFFT, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("GPUFFT.hsf", "SharedMemoryTwoForOneRealFFTCS");
        shaderLibrary->LoadShader(ShaderID::SharedMemoryTwoForOneRealFFT, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("GPUFFT.hsf", "SharedMemoryTwoForOneRealIFFTCS");
        shaderLibrary->LoadShader(ShaderID::SharedMemoryTwoForOneRealIFFT, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("GPUFFT.hsf", "SharedMemoryComplexFFTConvolutionCS");
        shaderLibrary->LoadShader(ShaderID::SharedMemoryComplexFFTConvolution, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/UIColorAndAlpha.hsf", "UIColorAndAlphaVS", "UIColorAndAlphaPS");
        shaderLibrary->LoadShader(ShaderID::UIColorAndAlpha, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/VisibilityBuffer.hsf", "VisibilityBufferVS", "VisibilityBufferPS");
        shaderLibrary->LoadShader(ShaderID::VBuffer, shaderDesc);

        shaderDesc = ShaderDesc::CreateMesh("RealTimeRenderer/VisibilityBufferMeshlet.hsf", "VisibilityBufferTS", "VisibilityBufferMS", "VisibilityBufferPS");
        shaderLibrary->LoadShader(ShaderID::VBufferMeshlet, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/BuildHZB.hsf", "BuildHZBCS");
        shaderDesc.AddDefine("BUILD_CLOSEST_HZB", 1);
        shaderDesc.AddDefine("BUILD_FURTHEST_HZB", 1);
        shaderLibrary->LoadShader(ShaderID::BuildHZB, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/ShadowMap.hsf", "ShadowMapVS", "ShadowMapPS");
        shaderDesc.AddDefine("SHADOW_MAP_TYPE", 0);
        shaderLibrary->LoadShader(ShaderID::CascadedShadowMap, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/ShadowMap.hsf", "ShadowMapVS", "ShadowMapPS");
        shaderDesc.AddDefine("SHADOW_MAP_TYPE", 1);
        shaderLibrary->LoadShader(ShaderID::CubeShadowMap, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/LocalLightShadows.hsf", "LocalLightShadowsVS", "LocalLightShadowsPS");
        shaderLibrary->LoadShader(ShaderID::LocalLightShadows, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceShadows.hsf", "ScreenSpaceShadowsDirectionalLightCS");
        shaderLibrary->LoadShader(ShaderID::ScreenSpaceShadowsDirectionalLight, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/GBuffer.hsf", "GBufferCS");
        shaderLibrary->LoadShader(ShaderID::GBuffer, shaderDesc);

        //{
        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflectionsTileClassification.hsf", "SSRTileClassificationHorizontalCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRTileClassificationHorizontal, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflectionsTileClassification.hsf", "SSRTileClassificationVerticalCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRTileClassificationVertical, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflectionsRayAllocation.hsf", "SSRRayAllocationCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRRayAllocation, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflections.hsf", "ScreenSpaceReflectionsCS");
        //    shaderDesc.AddDefine(HE_TEXT("SSR_EARLY_EXIT_RAYS"));
        //    shaderLibrary->LoadShader(ShaderID::SSRDispatchEarlyExitRays, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflections.hsf", "ScreenSpaceReflectionsCS");
        //    shaderDesc.AddDefine(HE_TEXT("SSR_CHEAP_RAYS"));
        //    shaderLibrary->LoadShader(ShaderID::SSRDispatchCheapRays, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflections.hsf", "ScreenSpaceReflectionsCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRDispatchExpensiveRays, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflectionsResolve.hsf", "ScreenSpaceReflectionsResolveCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRResolve, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflectionsTemporalFiltering.hsf", "SSRTemporalFilteringCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRTemporalFiltering, shaderDesc);

        //    shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceReflectionsSpatialFiltering.hsf", "SSRSpatialFilteringCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRSpatialFiltering, shaderDesc);
        //}

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/GTAOHorizonSearchAndIntegral.hsf", "GTAOHorizonSearchAndIntegralCS");
        shaderLibrary->LoadShader(ShaderID::GTAOHorizonSearchAndIntegral, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/GTAOSpatialFiltering.hsf", "GTAOSpatialFilteringCS");
        shaderLibrary->LoadShader(ShaderID::GTAOSpatialFiltering, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/ScreenSpaceAmbientOcclusion.hsf", "GTAOTemporalFilteringCS");
        shaderLibrary->LoadShader(ShaderID::GTAOTemporalFiltering, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/IndirectLightingDiffuse.hsf", "IndirectLightingDiffuseVS", "IndirectLightingDiffusePS");
        shaderLibrary->LoadShader(ShaderID::IndirectLightingDiffuse, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/IndirectLightingSpecular.hsf", "IndirectLightingSpecularVS", "IndirectLightingSpecularPS");
        shaderLibrary->LoadShader(ShaderID::IndirectLightingSpecular, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/MotionVectors.hsf", "MotionVectorsCS");
        shaderLibrary->LoadShader(ShaderID::MotionVectors, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/DirectLighting.hsf", "DirectLightingVS", "DirectLightingPS");
        shaderLibrary->LoadShader(ShaderID::DirectLighting, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/SkyBox.hsf", "SkyBoxVS", "SkyBoxPS");
        shaderLibrary->LoadShader(ShaderID::SkyBox, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringSetup.hsf", "SubsurfaceScatteringSetupCS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringSetup, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringClassifyTiles.hsf", "SubsurfaceScatteringClassifyTilesCS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringClassifyTiles, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringBuildIndirectArguments.hsf", "SubsurfaceScatteringBuildIndirectArgumentsCS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringBuildIndirectArguments, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringSampleDiffusionProfile.hsf", "SubsurfaceScatteringSampleDiffusionProfileCS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringSampleDiffusionProfile, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringComputeVariance.hsf", "SubsurfaceScatteringComputeVarianceCS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringComputeVariance, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringCopyResults.hsf", "SubsurfaceScatteringCopyResultsVS", "SubsurfaceScatteringCopyResultsPS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringCopyResults, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringRecombine.hsf", "SubsurfaceScatteringRecombineVS", "SubsurfaceScatteringRecombinePS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringRecombine, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIFreeSurfels.hsf", "SurfelGIFreeSurfelsCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIFreeSurfels, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIGapFilling.hsf", "SurfelGIGapFillingCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIGapFilling, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIIndirectAruguments.hsf", "SurfelGIIndirectArugumentsCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIIndirectAruguments, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIGridReset.hsf", "SurfelGIGridResetCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIGridReset, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIComputeCellCapacity.hsf", "SurfelGIComputeCellCapacityCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIComputeCellCapacity, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIComputeCellOffset.hsf", "SurfelGIComputeCellOffsetCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIComputeCellOffset, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/SurfelGI/SurfelGIVisualization.hsf", "SurfelGIVisualizationCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIVisualization, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/LightShaftsDownsample.hsf", "LightShaftsDownsampleCS");
        shaderLibrary->LoadShader(ShaderID::LightShaftsDownsample, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/LightShaftsRadialBlur.hsf", "LightShaftsRadialBlurCS");
        shaderLibrary->LoadShader(ShaderID::LightShaftsRadialBlur, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/LightShaftsApply.hsf", "FullScreenQuadVS", "LightShaftsApplyPS");
        //shaderDesc = ShaderDesc::CreateGraphics("FullScreenQuadVertexShader.hsf", "FullScreenQuadVS", "RealTimeRenderer/LightShaftsApply.hsf", "LightShaftsApplyPS");
        shaderLibrary->LoadShader(ShaderID::LightShaftsApply, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/TemporalSuperSampling.hsf", "TemporalSuperSamplingCS");
        shaderLibrary->LoadShader(ShaderID::TemporalSuperSampling, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/DepthOfFieldSetup.hsf", "DepthOfFieldSetupCS");
        shaderLibrary->LoadShader(ShaderID::DepthOfFieldSetup, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/DepthOfFieldGather.hsf", "DepthOfFieldGatherCS");
        shaderLibrary->LoadShader(ShaderID::DepthOfFieldGather, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/DepthOfFieldPostfilter.hsf", "DepthOfFieldPostfilterCS");
        shaderLibrary->LoadShader(ShaderID::DepthOfFieldPostfilter, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/DepthOfFieldRecombine.hsf", "DepthOfFieldRecombineCS");
        shaderLibrary->LoadShader(ShaderID::DepthOfFieldRecombine, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/AutoExposureBuildHistogram.hsf", "AutoExposureBuildHistogramCS");
        shaderLibrary->LoadShader(ShaderID::AutoExposureBuildHistogram, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/AutoExposureComputeExposure.hsf", "AutoExposureComputeExposureCS");
        shaderLibrary->LoadShader(ShaderID::AutoExposureComputeExposure, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/Downsample.hsf", "DownsampleCS");
        shaderLibrary->LoadShader(ShaderID::Downsample, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/GaussianBloomDownsample.hsf", "GaussianBloomDownsampleCS");
        shaderLibrary->LoadShader(ShaderID::GaussianBloomDownsample, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/GaussianBloomUpsample.hsf", "GaussianBloomUpsampleCS");
        shaderLibrary->LoadShader(ShaderID::GaussianBloomUpsample, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/ConvolutionBloomResizeKernel.hsf", "ConvolutionBloomResizeKernelCS");
        shaderLibrary->LoadShader(ShaderID::ConvolutionBloomResizeKernel, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/PostProcessing/LensFlaresGhost.hsf", "LensFlaresGhostVS", "LensFlaresGhostPS");
        shaderLibrary->LoadShader(ShaderID::LensFlaresGhost, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/LensFlaresTileCulling.hsf", "LensFlaresTileCullingCS");
        shaderLibrary->LoadShader(ShaderID::LensFlaresTileCulling, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/PostProcessing/LensFlaresGlare.hsf", "LensFlaresGlareVS", "LensFlaresGlarePS");
        shaderLibrary->LoadShader(ShaderID::LensFlaresGlare, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/PostProcessing/LensFlaresCombine.hsf", "LensFlaresCombineVS", "LensFlaresCombinePS");
        shaderLibrary->LoadShader(ShaderID::LensFlaresCombine, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/LocalExposureComputeLuminances.hsf", "LocalExposureComputeLuminancesCS");
        shaderLibrary->LoadShader(ShaderID::LocalExposureComputeLuminances, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/LocalExposureComputeWeights.hsf", "LocalExposureComputeWeightsCS");
        shaderLibrary->LoadShader(ShaderID::LocalExposureComputeWeights, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/LocalExposureBlendExposures.hsf", "LocalExposureBlendExposuresCS");
        shaderLibrary->LoadShader(ShaderID::LocalExposureBlendExposures, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/LocalExposureBlendLaplacian.hsf", "LocalExposureBlendLaplacianCS");
        shaderLibrary->LoadShader(ShaderID::LocalExposureBlendLaplacian, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/LocalExposureGuidedUpsampling.hsf", "LocalExposureGuidedUpsamplingCS");
        shaderLibrary->LoadShader(ShaderID::LocalExposureGuidedUpsampling, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/ColorLUT.hsf", "ColorLUTCS");
        shaderLibrary->LoadShader(ShaderID::ColorLUT, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/ToneMapping.hsf", "ToneMappingCS");
        shaderLibrary->LoadShader(ShaderID::ToneMapping, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/PostProcessing/EditorSelectionOutlineMaskGen.hsf", "EditorSelectionOutlineMaskGenVS", "EditorSelectionOutlineMaskGenPS");
        shaderLibrary->LoadShader(ShaderID::EditorSelectionOutlineMaskGen, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/EditorSelectionOutlineSetup.hsf", "EditorSelectionOutlineSetupCS");
        shaderLibrary->LoadShader(ShaderID::EditorSelectionOutlineSetup, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/EditorSelectionOutlineJumpFlood.hsf", "EditorSelectionOutlineJumpFloodCS");
        shaderLibrary->LoadShader(ShaderID::EditorSelectionOutlineJumpFlood, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/EditorSelectionOutlineComposite.hsf", "EditorSelectionOutlineCompositeCS");
        shaderLibrary->LoadShader(ShaderID::EditorSelectionOutlineComposite, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/VisualizePrimitiveID.hsf", "VisualizePrimitiveIDCS");
        shaderLibrary->LoadShader(ShaderID::VisualizePrimitiveID, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/VisualizeMaterialID.hsf", "VisualizeMaterialIDCS");
        shaderLibrary->LoadShader(ShaderID::VisualizeMaterialID, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/VisualizeWorldSpaceNormal.hsf", "VisualizeWorldSpaceNormalCS");
        shaderLibrary->LoadShader(ShaderID::VisualizeWorldSpaceNormal, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/VisualizeMotionVectors.hsf", "VisualizeMotionVectorsCS");
        shaderLibrary->LoadShader(ShaderID::VisualizeMotionVectors, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/VisualizeAmbientOcclusion.hsf", "VisualizeAmbientOcclusionCS");
        shaderLibrary->LoadShader(ShaderID::VisualizeAmbientOcclusion, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("RealTimeRenderer/PostProcessing/VisualizeScreenSpaceShadowMask.hsf", "VisualizeScreenSpaceShadowMaskCS");
        shaderLibrary->LoadShader(ShaderID::VisualizeScreenSpaceShadowMask, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/DebugDrawLines.hsf", "DebugDrawLinesVS", "DebugDrawLinesPS");
        shaderLibrary->LoadShader(ShaderID::DebugDraw, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/GUIComposition.hsf", "GUICompositionVS", "GUICompositionPS");
        shaderLibrary->LoadShader(ShaderID::GUIComposition, shaderDesc);
    }
}