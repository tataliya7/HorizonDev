#include "ShaderLibrary.h"

namespace Horizon
{
    static void LoadSkyAtmosphereShaders_Deprecated(ShaderLibrary* shaderLibrary)
    {
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SkyAtmosphereTransmittanceLut.hsm", "SkyAtmosphereTransmittanceLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_TRANSMITTANCE_LUT_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereTransmittanceLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SkyAtmosphereMultipleScatteringLut.hsm", "SkyAtmosphereMultipleScatteringLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_MULTIPLE_SCATTERING_LUT_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereMultipleScatteringLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SkyAtmosphereSkyViewLut.hsm", "SkyAtmosphereSkyViewLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_SKY_VIEW_LUT_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereSkyViewLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SkyAtmosphereAerialPerspectiveVolume.hsm", "SkyAtmosphereAerialPerspectiveVolumeCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_AERIAL_PERSPECTIVE_VOLUME_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereAerialPerspectiveVolume, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/SkyAtmosphereRayMarching.hsm", "SkyAtmosphereRayMarchingPS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_RAY_MARCHING_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereRayMarching, shaderDesc);
        }
    }

    void LoadAllShaders_Deprecated(ShaderLibrary* shaderLibrary)
    {
        LoadSkyAtmosphereShaders_Deprecated(shaderLibrary);

        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/PreIntegratedBRDF.hsm", "PreIntegratedBRDFCS");
            shaderLibrary->LoadShader(ShaderID::PreIntegratedBRDF, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/LatLongToCubemap.hsm", "LatLongToCubemapCS");
            shaderLibrary->LoadShader(ShaderID::LatLongToCubemap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/DownsampleCubemap.hsm", "DownsampleCubemapCS");
            shaderLibrary->LoadShader(ShaderID::DownsampleCubemap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/DownsampleTexture2D.hsm", "DownsampleTexture2DCS");
            shaderLibrary->LoadShader(ShaderID::DownsampleTexture2DCS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/DownsampleTexture2D.hsm", "DownsampleTexture2DVS");
            shaderLibrary->LoadShader(ShaderID::DownsampleTexture2DVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/DownsampleTexture2D.hsm", "DownsampleTexture2DPS");
            shaderLibrary->LoadShader(ShaderID::DownsampleTexture2DPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/ComputeEnvironmentIrradiance.hsm", "ComputeEnvironmentIrradianceCS");
            shaderLibrary->LoadShader(ShaderID::ComputeEnvironmentIrradiance, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/ComputeEnvironmentIrradianceSH.hsm", "ComputeEnvironmentIrradianceSHCS");
            shaderLibrary->LoadShader(ShaderID::ComputeEnvironmentIrradianceSH, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/FilterEnvironmentMap.hsm", "FilterEnvironmentMapCS");
            shaderLibrary->LoadShader(ShaderID::FilterEnvironmentMap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hsm", "SharedMemoryComplexFFTCS");
            shaderLibrary->LoadShader(ShaderID::SharedMemoryComplexFFT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hsm", "SharedMemoryComplexIFFTCS");
            shaderLibrary->LoadShader(ShaderID::SharedMemoryComplexIFFT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hsm", "SharedMemoryTwoForOneRealFFTCS");
            shaderLibrary->LoadShader(ShaderID::SharedMemoryTwoForOneRealFFT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hsm", "SharedMemoryTwoForOneRealIFFTCS");
            shaderLibrary->LoadShader(ShaderID::SharedMemoryTwoForOneRealIFFT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hsm", "SharedMemoryComplexFFTConvolutionCS");
            shaderLibrary->LoadShader(ShaderID::SharedMemoryComplexFFTConvolution, shaderDesc);
        }
        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/UIColorAndAlpha.hsm", "UIColorAndAlphaVS", "UIColorAndAlphaPS");
        shaderLibrary->LoadShader(ShaderID::UIColorAndAlpha, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/VisibilityBuffer.hsm", "VisibilityBufferVS", "VisibilityBufferPS");
        shaderLibrary->LoadShader(ShaderID::VBuffer, shaderDesc);

        shaderDesc = ShaderDesc::CreateMesh("RealTimeRenderer/VisibilityBufferMeshlet.hsm", "VisibilityBufferTS", "VisibilityBufferMS", "VisibilityBufferPS");
        shaderLibrary->LoadShader(ShaderID::VBufferMeshlet, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/BuildHZB.hsm", "BuildHZBCS");
        shaderDesc.AddDefine("BUILD_CLOSEST_HZB", 1);
        shaderDesc.AddDefine("BUILD_FURTHEST_HZB", 1);
        shaderLibrary->LoadShader(ShaderID::BuildHZB, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/ShadowMap.hsm", "ShadowMapVS", "ShadowMapPS");
        shaderDesc.AddDefine("SHADOW_MAP_TYPE", 0);
        shaderLibrary->LoadShader(ShaderID::CascadedShadowMap, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/ShadowMap.hsm", "ShadowMapVS", "ShadowMapPS");
        shaderDesc.AddDefine("SHADOW_MAP_TYPE", 1);
        shaderLibrary->LoadShader(ShaderID::CubeShadowMap, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/LocalLightShadows.hsm", "LocalLightShadowsVS", "LocalLightShadowsPS");
        shaderLibrary->LoadShader(ShaderID::LocalLightShadows, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/ScreenSpaceShadows.hsm", "ScreenSpaceShadowsDirectionalLightCS");
        shaderLibrary->LoadShader(ShaderID::ScreenSpaceShadowsDirectionalLight, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/GBuffer.hsm", "GBufferCS");
        shaderLibrary->LoadShader(ShaderID::GBuffer, shaderDesc);

        //{
        //    {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/ScreenSpaceReflectionsTileClassification.hsm", "SSRTileClassificationHorizontalCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRTileClassificationHorizontal, shaderDesc);

        //    {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/ScreenSpaceReflectionsTileClassification.hsm", "SSRTileClassificationVerticalCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRTileClassificationVertical, shaderDesc);

        //    {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/ScreenSpaceReflectionsRayAllocation.hsm", "SSRRayAllocationCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRRayAllocation, shaderDesc);

        //    {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/ScreenSpaceReflections.hsm", "ScreenSpaceReflectionsCS");
        //    shaderDesc.AddDefine(HE_TEXT("SSR_EARLY_EXIT_RAYS"));
        //    shaderLibrary->LoadShader(ShaderID::SSRDispatchEarlyExitRays, shaderDesc);

        //    {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/ScreenSpaceReflections.hsm", "ScreenSpaceReflectionsCS");
        //    shaderDesc.AddDefine(HE_TEXT("SSR_CHEAP_RAYS"));
        //    shaderLibrary->LoadShader(ShaderID::SSRDispatchCheapRays, shaderDesc);

        //    {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/ScreenSpaceReflections.hsm", "ScreenSpaceReflectionsCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRDispatchExpensiveRays, shaderDesc);

        //    {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/ScreenSpaceReflectionsResolve.hsm", "ScreenSpaceReflectionsResolveCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRResolve, shaderDesc);

        //    {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/ScreenSpaceReflectionsTemporalFiltering.hsm", "SSRTemporalFilteringCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRTemporalFiltering, shaderDesc);

        //    {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/ScreenSpaceReflectionsSpatialFiltering.hsm", "SSRSpatialFilteringCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRSpatialFiltering, shaderDesc);
        //}

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/GTAOHorizonSearchAndIntegral.hsm", "GTAOHorizonSearchAndIntegralCS");
        shaderLibrary->LoadShader(ShaderID::GTAOHorizonSearchAndIntegral, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/GTAOSpatialFiltering.hsm", "GTAOSpatialFilteringCS");
        shaderLibrary->LoadShader(ShaderID::GTAOSpatialFiltering, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/ScreenSpaceAmbientOcclusion.hsm", "GTAOTemporalFilteringCS");
        shaderLibrary->LoadShader(ShaderID::GTAOTemporalFiltering, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/IndirectLightingDiffuse.hsm", "IndirectLightingDiffuseVS", "IndirectLightingDiffusePS");
        shaderLibrary->LoadShader(ShaderID::IndirectLightingDiffuse, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/IndirectLightingSpecular.hsm", "IndirectLightingSpecularVS", "IndirectLightingSpecularPS");
        shaderLibrary->LoadShader(ShaderID::IndirectLightingSpecular, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/MotionVectors.hsm", "MotionVectorsCS");
        shaderLibrary->LoadShader(ShaderID::MotionVectors, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/DirectLighting.hsm", "DirectLightingVS", "DirectLightingPS");
        shaderLibrary->LoadShader(ShaderID::DirectLighting, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/SkyBox.hsm", "SkyBoxVS", "SkyBoxPS");
        shaderLibrary->LoadShader(ShaderID::SkyBox, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringSetup.hsm", "SubsurfaceScatteringSetupCS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringSetup, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringClassifyTiles.hsm", "SubsurfaceScatteringClassifyTilesCS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringClassifyTiles, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringBuildIndirectArguments.hsm", "SubsurfaceScatteringBuildIndirectArgumentsCS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringBuildIndirectArguments, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringSampleDiffusionProfile.hsm", "SubsurfaceScatteringSampleDiffusionProfileCS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringSampleDiffusionProfile, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringComputeVariance.hsm", "SubsurfaceScatteringComputeVarianceCS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringComputeVariance, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringCopyResults.hsm", "SubsurfaceScatteringCopyResultsVS", "SubsurfaceScatteringCopyResultsPS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringCopyResults, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringRecombine.hsm", "SubsurfaceScatteringRecombineVS", "SubsurfaceScatteringRecombinePS");
        shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringRecombine, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/SurfelGI/SurfelGIFreeSurfels.hsm", "SurfelGIFreeSurfelsCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIFreeSurfels, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/SurfelGI/SurfelGIGapFilling.hsm", "SurfelGIGapFillingCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIGapFilling, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/SurfelGI/SurfelGIIndirectArguments.hsm", "SurfelGIIndirectArgumentsCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIIndirectArguments, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/SurfelGI/SurfelGIGridReset.hsm", "SurfelGIGridResetCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIGridReset, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/SurfelGI/SurfelGIComputeCellCapacity.hsm", "SurfelGIComputeCellCapacityCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIComputeCellCapacity, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/SurfelGI/SurfelGIComputeCellOffset.hsm", "SurfelGIComputeCellOffsetCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIComputeCellOffset, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/SurfelGI/SurfelGIVisualization.hsm", "SurfelGIVisualizationCS");
        shaderLibrary->LoadShader(ShaderID::SurfelGIVisualization, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/LightShaftsDownsample.hsm", "LightShaftsDownsampleCS");
        shaderLibrary->LoadShader(ShaderID::LightShaftsDownsample, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/LightShaftsRadialBlur.hsm", "LightShaftsRadialBlurCS");
        shaderLibrary->LoadShader(ShaderID::LightShaftsRadialBlur, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/LightShaftsApply.hsm", "FullScreenQuadVS", "LightShaftsApplyPS");
        //shaderDesc = ShaderDesc::CreateGraphics("Shaders/FullScreenQuadVertexShader.hsm", "FullScreenQuadVS", "RealTimeRenderer/LightShaftsApply.hsm", "LightShaftsApplyPS");
        shaderLibrary->LoadShader(ShaderID::LightShaftsApply, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/TemporalSuperSampling.hsm", "TemporalSuperSamplingCS");
        shaderLibrary->LoadShader(ShaderID::TemporalSuperSampling, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/DepthOfFieldSetup.hsm", "DepthOfFieldSetupCS");
        shaderLibrary->LoadShader(ShaderID::DepthOfFieldSetup, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/DepthOfFieldGather.hsm", "DepthOfFieldGatherCS");
        shaderLibrary->LoadShader(ShaderID::DepthOfFieldGather, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/DepthOfFieldPostfilter.hsm", "DepthOfFieldPostfilterCS");
        shaderLibrary->LoadShader(ShaderID::DepthOfFieldPostfilter, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/DepthOfFieldRecombine.hsm", "DepthOfFieldRecombineCS");
        shaderLibrary->LoadShader(ShaderID::DepthOfFieldRecombine, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/AutoExposureBuildHistogram.hsm", "AutoExposureBuildHistogramCS");
        shaderLibrary->LoadShader(ShaderID::AutoExposureBuildHistogram, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/AutoExposureComputeExposure.hsm", "AutoExposureComputeExposureCS");
        shaderLibrary->LoadShader(ShaderID::AutoExposureComputeExposure, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/Downsample.hsm", "DownsampleCS");
        shaderLibrary->LoadShader(ShaderID::Downsample, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/GaussianBloomDownsample.hsm", "GaussianBloomDownsampleCS");
        shaderLibrary->LoadShader(ShaderID::GaussianBloomDownsample, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/GaussianBloomUpsample.hsm", "GaussianBloomUpsampleCS");
        shaderLibrary->LoadShader(ShaderID::GaussianBloomUpsample, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/ConvolutionBloomResizeKernel.hsm", "ConvolutionBloomResizeKernelCS");
        shaderLibrary->LoadShader(ShaderID::ConvolutionBloomResizeKernel, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/PostProcessing/LensFlaresGhost.hsm", "LensFlaresGhostVS", "LensFlaresGhostPS");
        shaderLibrary->LoadShader(ShaderID::LensFlaresGhost, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/LensFlaresTileCulling.hsm", "LensFlaresTileCullingCS");
        shaderLibrary->LoadShader(ShaderID::LensFlaresTileCulling, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/PostProcessing/LensFlaresGlare.hsm", "LensFlaresGlareVS", "LensFlaresGlarePS");
        shaderLibrary->LoadShader(ShaderID::LensFlaresGlare, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/PostProcessing/LensFlaresCombine.hsm", "LensFlaresCombineVS", "LensFlaresCombinePS");
        shaderLibrary->LoadShader(ShaderID::LensFlaresCombine, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/LocalExposureComputeLuminance.hsm", "LocalExposureComputeLuminanceCS");
        shaderLibrary->LoadShader(ShaderID::LocalExposureComputeLuminance, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/LocalExposureComputeWeights.hsm", "LocalExposureComputeWeightsCS");
        shaderLibrary->LoadShader(ShaderID::LocalExposureComputeWeights, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/LocalExposureBlendExposures.hsm", "LocalExposureBlendExposuresCS");
        shaderLibrary->LoadShader(ShaderID::LocalExposureBlendExposures, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/LocalExposureBlendLaplacian.hsm", "LocalExposureBlendLaplacianCS");
        shaderLibrary->LoadShader(ShaderID::LocalExposureBlendLaplacian, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/LocalExposureGuidedUpsampling.hsm", "LocalExposureGuidedUpsamplingCS");
        shaderLibrary->LoadShader(ShaderID::LocalExposureGuidedUpsampling, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/ColorLUT.hsm", "ColorLUTCS");
        shaderLibrary->LoadShader(ShaderID::ColorLUT, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/ToneMapping.hsm", "ToneMappingCS");
        shaderLibrary->LoadShader(ShaderID::ToneMapping, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("Shaders/RealTimeRenderer/PostProcessing/EditorSelectionOutlineMaskGen.hsm", "EditorSelectionOutlineMaskGenVS", "EditorSelectionOutlineMaskGenPS");
        shaderLibrary->LoadShader(ShaderID::EditorSelectionOutlineMaskGen, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/EditorSelectionOutlineSetup.hsm", "EditorSelectionOutlineSetupCS");
        shaderLibrary->LoadShader(ShaderID::EditorSelectionOutlineSetup, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/EditorSelectionOutlineJumpFlood.hsm", "EditorSelectionOutlineJumpFloodCS");
        shaderLibrary->LoadShader(ShaderID::EditorSelectionOutlineJumpFlood, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/EditorSelectionOutlineComposite.hsm", "EditorSelectionOutlineCompositeCS");
        shaderLibrary->LoadShader(ShaderID::EditorSelectionOutlineComposite, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/VisualizePrimitiveID.hsm", "VisualizePrimitiveIDCS");
        shaderLibrary->LoadShader(ShaderID::VisualizePrimitiveID, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/VisualizeMaterialID.hsm", "VisualizeMaterialIDCS");
        shaderLibrary->LoadShader(ShaderID::VisualizeMaterialID, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/VisualizeWorldSpaceNormal.hsm", "VisualizeWorldSpaceNormalCS");
        shaderLibrary->LoadShader(ShaderID::VisualizeWorldSpaceNormal, shaderDesc);

        {shaderDesc = ShaderDesc::CreateCompute("Shaders/RealTimeRenderer/PostProcessing/VisualizeMotionVectors.hsm", "VisualizeMotionVectorsCS");
        shaderLibrary->LoadShader(ShaderID::VisualizeMotionVectors, shaderDesc);

        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/VisualizeAmbientOcclusion.hsm", "VisualizeAmbientOcclusionCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeAmbientOcclusion, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/VisualizeAmbientOcclusion.hsm", "VisualizeAmbientOcclusionCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeAmbientOcclusion, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/VisualizeScreenSpaceShadowMask.hsm", "VisualizeScreenSpaceShadowMaskCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeScreenSpaceShadowMask, shaderDesc);
        }

        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/GUIComposition.hsm", "GUICompositionVS");
            shaderLibrary->LoadShader(ShaderID::GUIComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/GUIComposition.hsm", "GUICompositionPS");
            shaderLibrary->LoadShader(ShaderID::GUIComposition, shaderDesc);
        }
    }
}