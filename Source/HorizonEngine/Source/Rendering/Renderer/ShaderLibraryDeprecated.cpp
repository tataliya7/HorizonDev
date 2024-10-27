#include "ShaderLibrary.h"

namespace Horizon
{
    static void LoadSkyAtmosphereShaders_Deprecated(ShaderLibrary* shaderLibrary)
    {
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SkyAtmosphere/SkyAtmosphereTransmittanceLut.hsm", "SkyAtmosphereTransmittanceLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_TRANSMITTANCE_LUT_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereTransmittanceLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SkyAtmosphere/SkyAtmosphereMultipleScatteringLut.hsm", "SkyAtmosphereMultipleScatteringLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_MULTIPLE_SCATTERING_LUT_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereMultipleScatteringLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SkyAtmosphere/SkyAtmosphereSkyViewLut.hsm", "SkyAtmosphereSkyViewLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_SKY_VIEW_LUT_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereSkyViewLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SkyAtmosphere/SkyAtmosphereAerialPerspectiveVolume.hsm", "SkyAtmosphereAerialPerspectiveVolumeCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_AERIAL_PERSPECTIVE_VOLUME_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereAerialPerspectiveVolume, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/SkyAtmosphere/SkyAtmosphereRayMarching.hsm", "SkyAtmosphereRayMarchingPS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_RAY_MARCHING_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereRayMarching, shaderDesc);
        }
    }

    void LoadAllShaders_Deprecated(ShaderLibrary* shaderLibrary)
    {
        LoadSkyAtmosphereShaders_Deprecated(shaderLibrary);

        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/ImGui.hsm", "ImGuiVS");
            shaderLibrary->LoadShader(ShaderID::ImGuiVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/ImGui.hsm", "ImGuiPS");
            shaderLibrary->LoadShader(ShaderID::ImGuiPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/FullScreenQuad.hsm", "FullScreenQuadVS");
            shaderLibrary->LoadShader(ShaderID::FullScreenQuadVS, shaderDesc);
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
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ImageBasedLighting/EnvironmentBRDFIntegration.hsm", "EnvironmentBRDFIntegrationCS");
            shaderLibrary->LoadShader(ShaderID::EnvironmentBRDFIntegration, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ImageBasedLighting/EnvironmentMapConvolution.hsm", "EnvironmentMapConvolutionCS");
            shaderLibrary->LoadShader(ShaderID::EnvironmentMapConvolution, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ImageBasedLighting/IrradianceEnvironmentMapReference.hsm", "IrradianceEnvironmentMapReferenceCS");
            shaderLibrary->LoadShader(ShaderID::IrradianceEnvironmentMapReference, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ImageBasedLighting/IrradianceEnvironmentMapSHOnePass.hsm", "IrradianceEnvironmentMapSHOnePassCS");
            shaderLibrary->LoadShader(ShaderID::IrradianceEnvironmentMapSHOnePass, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ImageBasedLighting/IrradianceEnvironmentMapSHSampling.hsm", "IrradianceEnvironmentMapSHSamplingCS");
        //     shaderLibrary->LoadShader(ShaderID::IrradianceEnvironmentMapSHSampling, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ImageBasedLighting/IrradianceEnvironmentMapSHIntegration.hsm", "IrradianceEnvironmentMapSHIntegrationCS");
        //     shaderLibrary->LoadShader(ShaderID::IrradianceEnvironmentMapSHIntegration, shaderDesc);
        // }
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
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/VisibilityBuffer.hsm", "VisibilityBufferVS");
            shaderLibrary->LoadShader(ShaderID::VisibilityBufferVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/VisibilityBuffer.hsm", "VisibilityBufferPS");
            shaderLibrary->LoadShader(ShaderID::VisibilityBufferPS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Amplification, "Shaders/RealTimeRenderer/VisibilityBufferMeshShading.hsm", "VisibilityBufferAS");
            //shaderLibrary->LoadShader(ShaderID::VisibilityBufferMeshShadingAS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Mesh, "Shaders/RealTimeRenderer/VisibilityBufferMeshShading.hsm", "VisibilityBufferMS");
            //shaderLibrary->LoadShader(ShaderID::VisibilityBufferMeshShadingMS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/VisibilityBufferMeshShading.hsm", "VisibilityBufferPS");
            //shaderLibrary->LoadShader(ShaderID::VisibilityBufferMeshShadingPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/MotionVectors.hsm", "MotionVectorsCS");
            shaderLibrary->LoadShader(ShaderID::MotionVectors, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/BuildDepthPyramid.hsm", "BuildDepthPyramidCS");
            shaderLibrary->LoadShader(ShaderID::BuildDepthPyramid, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/GBuffer.hsm", "GBufferCS");
            shaderLibrary->LoadShader(ShaderID::GBuffer, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/LocalLightCullingClearBuffer.hsm", "LocalLightCullingClearBufferCS");
            shaderLibrary->LoadShader(ShaderID::LocalLightCullingClearBuffer, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/LocalLightCulling.hsm", "LocalLightCullingCS");
            shaderLibrary->LoadShader(ShaderID::LocalLightCulling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/LocalLightCullingDebug.hsm", "LocalLightCullingDebugCS");
            shaderLibrary->LoadShader(ShaderID::LocalLightCullingDebug, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/CascadedShadowMap.hsm", "CascadedShadowMapVS");
            shaderLibrary->LoadShader(ShaderID::CascadedShadowMapVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/CascadedShadowMap.hsm", "CascadedShadowMapPS");
            shaderLibrary->LoadShader(ShaderID::CascadedShadowMapPS, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/LocalLightShadows.hsm", "LocalLightShadowsVS");
        //     shaderLibrary->LoadShader(ShaderID::LocalLightShadowsVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/LocalLightShadows.hsm", "LocalLightShadowsPS");
        //     shaderLibrary->LoadShader(ShaderID::LocalLightShadowsPS, shaderDesc);
        // }

        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceShadows.hsm", "ScreenSpaceShadowsForDistantLightCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceShadowsForDistantLight, shaderDesc);
        }
        //{
        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflectionsTileClassification.hsm", "SSRTileClassificationHorizontalCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRTileClassificationHorizontal, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflectionsTileClassification.hsm", "SSRTileClassificationVerticalCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRTileClassificationVertical, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflectionsRayAllocation.hsm", "SSRRayAllocationCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRRayAllocation, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflections.hsm", "ScreenSpaceReflectionsCS");
        //    shaderDesc.AddDefine(HE_TEXT("SSR_EARLY_EXIT_RAYS"));
        //    shaderLibrary->LoadShader(ShaderID::SSRDispatchEarlyExitRays, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflections.hsm", "ScreenSpaceReflectionsCS");
        //    shaderDesc.AddDefine(HE_TEXT("SSR_CHEAP_RAYS"));
        //    shaderLibrary->LoadShader(ShaderID::SSRDispatchCheapRays, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflections.hsm", "ScreenSpaceReflectionsCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRDispatchExpensiveRays, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflectionsResolve.hsm", "ScreenSpaceReflectionsResolveCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRResolve, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflectionsTemporalFiltering.hsm", "SSRTemporalFilteringCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRTemporalFiltering, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflectionsSpatialFiltering.hsm", "SSRSpatialFilteringCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRSpatialFiltering, shaderDesc);
        //}
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/GTAOHorizonSearchAndIntegral.hsm", "GTAOHorizonSearchAndIntegralCS");
        //     shaderLibrary->LoadShader(ShaderID::GTAOHorizonSearchAndIntegral, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/GTAOSpatialFiltering.hsm", "GTAOSpatialFilteringCS");
        //     shaderLibrary->LoadShader(ShaderID::GTAOSpatialFiltering, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceAmbientOcclusion.hsm", "GTAOTemporalFilteringCS");
        //     shaderLibrary->LoadShader(ShaderID::GTAOTemporalFiltering, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/IndirectLightingDiffuse.hsm", "IndirectLightingDiffusePS");
            shaderLibrary->LoadShader(ShaderID::IndirectLightingDiffuse, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/IndirectLightingSpecular.hsm", "IndirectLightingSpecularPS");
            shaderLibrary->LoadShader(ShaderID::IndirectLightingSpecular, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/DirectLighting.hsm", "DirectLightingPS");
            shaderLibrary->LoadShader(ShaderID::DirectLighting, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/SkyBox.hsm", "SkyBoxVS");
        //     shaderLibrary->LoadShader(ShaderID::SkyBoxVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/SkyBox.hsm", "SkyBoxPS");
        //     shaderLibrary->LoadShader(ShaderID::SkyBoxPS, shaderDesc);
        // }
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringInitialize.hsm", "SubsurfaceScatteringInitializeCS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringInitialize, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringClassifyTiles.hsm", "SubsurfaceScatteringClassifyTilesCS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringClassifyTiles, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringBuildIndirectArguments.hsm", "SubsurfaceScatteringBuildIndirectArgumentsCS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringBuildIndirectArguments, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringSampleDiffusionProfile.hsm", "SubsurfaceScatteringSampleDiffusionProfileCS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringSampleDiffusionProfile, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringComputeVariance.hsm", "SubsurfaceScatteringComputeVarianceCS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringComputeVariance, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringCopyResults.hsm", "SubsurfaceScatteringCopyResultsVS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringCopyResultsVS, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringCopyResults.hsm", "SubsurfaceScatteringCopyResultsPS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringCopyResultsPS, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringRecombine.hsm", "SubsurfaceScatteringRecombineVS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringRecombineVS, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringRecombine.hsm", "SubsurfaceScatteringRecombinePS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringRecombinePS, shaderDesc);
        //}
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIFreeSurfels.hsm", "SurfelGIFreeSurfelsCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIFreeSurfels, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIGapFilling.hsm", "SurfelGIGapFillingCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIGapFilling, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIIndirectArguments.hsm", "SurfelGIIndirectArgumentsCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIIndirectArguments, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIGridReset.hsm", "SurfelGIGridResetCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIGridReset, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIComputeCellCapacity.hsm", "SurfelGIComputeCellCapacityCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIComputeCellCapacity, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIComputeCellOffset.hsm", "SurfelGIComputeCellOffsetCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIComputeCellOffset, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIVisualization.hsm", "SurfelGIVisualizationCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIVisualization, shaderDesc);
        // }

        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/LightShaftsDownsample.hsm", "LightShaftsDownsampleCS");
        //     shaderLibrary->LoadShader(ShaderID::LightShaftsDownsample, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/LightShaftsRadialBlur.hsm", "LightShaftsRadialBlurCS");
        //     shaderLibrary->LoadShader(ShaderID::LightShaftsRadialBlur, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/LightShaftsApply.hsm", "LightShaftsApplyPS");
        //     shaderLibrary->LoadShader(ShaderID::LightShaftsApply, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VolumetricFogVoxelization.hsm", "VolumetricFogVoxelizationCS");
            shaderLibrary->LoadShader(ShaderID::VolumetricFogVoxelization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VolumetricFogLightScattering.hsm", "VolumetricFogLightScatteringCS");
            shaderLibrary->LoadShader(ShaderID::VolumetricFogLightScattering, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VolumetricFogFinalIntegration.hsm", "VolumetricFogFinalIntegrationCS");
            shaderLibrary->LoadShader(ShaderID::VolumetricFogFinalIntegration, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/VolumetricFogComposition.hsm", "VolumetricFogCompositionPS");
            shaderLibrary->LoadShader(ShaderID::VolumetricFogComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/LocalFogVolume/LocalFogVolume.hsm", "LocalFogVolumeVS");
            shaderLibrary->LoadShader(ShaderID::LocalFogVolumeVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/LocalFogVolume/LocalFogVolume.hsm", "LocalFogVolumePS");
            shaderLibrary->LoadShader(ShaderID::LocalFogVolumePS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/MotionBlurSetup.hsm", "MotionBlurSetupCS");
            shaderLibrary->LoadShader(ShaderID::MotionBlurSetupCS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/PostProcessing/MotionBlurVelocityDilation.hsm", "MotionBlurVelocityDilationScatterVS");
            shaderLibrary->LoadShader(ShaderID::MotionBlurVelocityDilationScatterVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/PostProcessing/MotionBlurVelocityDilation.hsm", "MotionBlurVelocityDilationScatterPS");
            shaderLibrary->LoadShader(ShaderID::MotionBlurVelocityDilationScatterPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/MotionBlurReconstructionFilter.hsm", "MotionBlurReconstructionFilterCS");
            shaderLibrary->LoadShader(ShaderID::MotionBlurReconstructionFilterCS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/TemporalSuperSampling.hsm", "TemporalSuperSamplingCS");
            //shaderLibrary->LoadShader(ShaderID::TemporalSuperSampling, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/DepthOfFieldSetup.hsm", "DepthOfFieldSetupCS");
            //shaderLibrary->LoadShader(ShaderID::DepthOfFieldSetup, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/DepthOfFieldGather.hsm", "DepthOfFieldGatherCS");
            //shaderLibrary->LoadShader(ShaderID::DepthOfFieldGather, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/DepthOfFieldPostfilter.hsm", "DepthOfFieldPostfilterCS");
            //shaderLibrary->LoadShader(ShaderID::DepthOfFieldPostfilter, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/DepthOfFieldRecombine.hsm", "DepthOfFieldRecombineCS");
            //shaderLibrary->LoadShader(ShaderID::DepthOfFieldRecombine, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/AutoExposureBuildHistogram.hsm", "AutoExposureBuildHistogramCS");
            shaderLibrary->LoadShader(ShaderID::AutoExposureBuildHistogram, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/AutoExposureComputeExposure.hsm", "AutoExposureComputeExposureCS");
            shaderLibrary->LoadShader(ShaderID::AutoExposureComputeExposure, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/CopyExposure.hsm", "CopyExposureCS");
            shaderLibrary->LoadShader(ShaderID::CopyExposure, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/BuildColorPyramid.hsm", "BuildColorPyramidCS");
            shaderLibrary->LoadShader(ShaderID::BuildColorPyramid, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/GaussianBloomDownsample.hsm", "GaussianBloomDownsampleCS");
            shaderLibrary->LoadShader(ShaderID::GaussianBloomDownsample, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/GaussianBloomUpsample.hsm", "GaussianBloomUpsampleCS");
            shaderLibrary->LoadShader(ShaderID::GaussianBloomUpsample, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/ConvolutionBloomResizeKernel.hsm", "ConvolutionBloomResizeKernelCS");
            shaderLibrary->LoadShader(ShaderID::ConvolutionBloomResizeKernel, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/PostProcessing/LensFlareGhost.hsm", "LensFlareGhostPS");
            shaderLibrary->LoadShader(ShaderID::LensFlareGhost, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LensFlareTileCulling.hsm", "LensFlareTileCullingCS");
        //     shaderLibrary->LoadShader(ShaderID::LensFlareTileCulling, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/PostProcessing/LensFlareGlare.hsm", "LensFlareGlareVS");
        //     shaderLibrary->LoadShader(ShaderID::LensFlareGlareVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/PostProcessing/LensFlareGlare.hsm", "LensFlareGlarePS");
        //     shaderLibrary->LoadShader(ShaderID::LensFlareGlarePS, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/PostProcessing/LensFlareCombine.hsm", "LensFlareCombinePS");
            shaderLibrary->LoadShader(ShaderID::LensFlareCombine, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalExposureComputeLuminance.hsm", "LocalExposureComputeLuminanceCS");
        //     shaderLibrary->LoadShader(ShaderID::LocalExposureComputeLuminance, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalExposureComputeWeights.hsm", "LocalExposureComputeWeightsCS");
        //     shaderLibrary->LoadShader(ShaderID::LocalExposureComputeWeights, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalExposureBlendExposures.hsm", "LocalExposureBlendExposuresCS");
        //     shaderLibrary->LoadShader(ShaderID::LocalExposureBlendExposures, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalExposureBlendLaplacian.hsm", "LocalExposureBlendLaplacianCS");
        //     shaderLibrary->LoadShader(ShaderID::LocalExposureBlendLaplacian, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalExposureGuidedUpsampling.hsm", "LocalExposureGuidedUpsamplingCS");
        //     shaderLibrary->LoadShader(ShaderID::LocalExposureGuidedUpsampling, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/ColorLUT.hsm", "ColorLUTCS");
            shaderLibrary->LoadShader(ShaderID::ColorLUT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/ToneMapping.hsm", "ToneMappingCS");
            shaderLibrary->LoadShader(ShaderID::ToneMapping, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/PostProcessing/SelectionOutlineMask.hsm", "SelectionOutlineMaskVS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineMaskVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/PostProcessing/SelectionOutlineMask.hsm", "SelectionOutlineMaskPS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineMaskPS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/SelectionOutlineSetup.hsm", "SelectionOutlineSetupCS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineSetup, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/SelectionOutlineJumpFlood.hsm", "SelectionOutlineJumpFloodCS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineJumpFlood, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/SelectionOutlineComposite.hsm", "SelectionOutlineCompositeCS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineComposite, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeDepth.hsm", "VisualizeDepthCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeDepth, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizePrimitiveID.hsm", "VisualizePrimitiveIDCS");
            shaderLibrary->LoadShader(ShaderID::VisualizePrimitiveID, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeMaterialID.hsm", "VisualizeMaterialIDCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeMaterialID, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeWorldSpaceNormal.hsm", "VisualizeWorldSpaceNormalCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeWorldSpaceNormal, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeMotionVectors.hsm", "VisualizeMotionVectorsCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeMotionVectors, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeAmbientOcclusion.hsm", "VisualizeAmbientOcclusionCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeAmbientOcclusion, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeScreenSpaceShadowMask.hsm", "VisualizeScreenSpaceShadowMaskCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeScreenSpaceShadowMask, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/GUIComposition.hsm", "GUICompositionPS");
            shaderLibrary->LoadShader(ShaderID::GUICompositionPS, shaderDesc);
        }
    }
}