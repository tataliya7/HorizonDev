#include "ShaderCollection.h"
#include "RealTimeRenderer/RealTimeRenderer.h"

namespace Horizon
{
    static void LoadSkyAtmosphereShaders_Deprecated(ShaderCollection* shaderLibrary)
    {
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SkyAtmosphere/SkyAtmosphereTransmittanceLut.hslib", "SkyAtmosphereTransmittanceLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_TRANSMITTANCE_LUT_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereTransmittanceLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SkyAtmosphere/SkyAtmosphereMultipleScatteringLut.hslib", "SkyAtmosphereMultipleScatteringLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_MULTIPLE_SCATTERING_LUT_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereMultipleScatteringLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SkyAtmosphere/SkyAtmosphereSkyViewLut.hslib", "SkyAtmosphereSkyViewLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_SKY_VIEW_LUT_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereSkyViewLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SkyAtmosphere/SkyAtmosphereAerialPerspectiveVolume.hslib", "SkyAtmosphereAerialPerspectiveVolumeCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_AERIAL_PERSPECTIVE_VOLUME_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereAerialPerspectiveVolume, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/SkyAtmosphere/SkyAtmosphereRayMarching.hslib", "SkyAtmosphereRayMarchingPS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_RAY_MARCHING_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereRayMarching, shaderDesc);
        }
    }

    void LoadAllShaders_Deprecated(ShaderCollection* shaderLibrary)
    {
        LoadSkyAtmosphereShaders_Deprecated(shaderLibrary);

        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/ImGui.hslib", "ImGuiVS");
            shaderLibrary->LoadShader(ShaderID::ImGuiVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/ImGui.hslib", "ImGuiPS");
            shaderLibrary->LoadShader(ShaderID::ImGuiPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/DrawFullscreenQuad.hslib", "DrawFullscreenQuadVS");
            shaderLibrary->LoadShader(ShaderID::DrawFullscreenQuadVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/LatLongToCubemap.hslib", "LatLongToCubemapCS");
            shaderLibrary->LoadShader(ShaderID::LatLongToCubemap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/DownsampleCubemap.hslib", "DownsampleCubemapCS");
            shaderLibrary->LoadShader(ShaderID::DownsampleCubemap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/DownsampleTexture2D.hslib", "DownsampleTexture2DCS");
            shaderLibrary->LoadShader(ShaderID::DownsampleTexture2DCS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/DownsampleTexture2D.hslib", "DownsampleTexture2DVS");
            shaderLibrary->LoadShader(ShaderID::DownsampleTexture2DVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/DownsampleTexture2D.hslib", "DownsampleTexture2DPS");
            shaderLibrary->LoadShader(ShaderID::DownsampleTexture2DPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ImageBasedLighting/EnvironmentBRDFIntegration.hslib", "EnvironmentBRDFIntegrationCS");
            shaderLibrary->LoadShader(ShaderID::EnvironmentBRDFIntegration, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ImageBasedLighting/EnvironmentMapConvolution.hslib", "EnvironmentMapConvolutionCS");
            shaderLibrary->LoadShader(ShaderID::EnvironmentMapConvolution, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ImageBasedLighting/IrradianceEnvironmentMapReference.hslib", "IrradianceEnvironmentMapReferenceCS");
            shaderLibrary->LoadShader(ShaderID::IrradianceEnvironmentMapReference, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ImageBasedLighting/IrradianceEnvironmentMapSHOnePass.hslib", "IrradianceEnvironmentMapSHOnePassCS");
            shaderLibrary->LoadShader(ShaderID::IrradianceEnvironmentMapSHOnePass, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ImageBasedLighting/IrradianceEnvironmentMapSHSampling.hslib", "IrradianceEnvironmentMapSHSamplingCS");
        //     shaderLibrary->LoadShader(ShaderID::IrradianceEnvironmentMapSHSampling, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ImageBasedLighting/IrradianceEnvironmentMapSHIntegration.hslib", "IrradianceEnvironmentMapSHIntegrationCS");
        //     shaderLibrary->LoadShader(ShaderID::IrradianceEnvironmentMapSHIntegration, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hslib", "SharedMemoryComplexFFTCS");
            shaderLibrary->LoadShader(ShaderID::SharedMemoryComplexFFT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hslib", "SharedMemoryComplexIFFTCS");
            shaderLibrary->LoadShader(ShaderID::SharedMemoryComplexIFFT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hslib", "SharedMemoryTwoForOneRealFFTCS");
            shaderLibrary->LoadShader(ShaderID::SharedMemoryTwoForOneRealFFT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hslib", "SharedMemoryTwoForOneRealIFFTCS");
            shaderLibrary->LoadShader(ShaderID::SharedMemoryTwoForOneRealIFFT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hslib", "SharedMemoryComplexFFTConvolutionCS");
            shaderLibrary->LoadShader(ShaderID::SharedMemoryComplexFFTConvolution, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VisibilityCulling.hslib", "IndirectArgumentInitializationCS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", IndexCountPerMeshlet);
            shaderDesc.AddDefine("INDIRECT_ARGUMENT_INITIALIZATION", 1);
            shaderLibrary->LoadShader(ShaderID::VisibilityCullingIndirectArgumentInitialization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VisibilityCulling.hslib", "InstanceCullingCS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", IndexCountPerMeshlet);
            shaderDesc.AddDefine("INSTANCE_CULLING", 1);
            shaderLibrary->LoadShader(ShaderID::VirtualGeometryInstanceCulling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VisibilityCulling.hslib", "MeshletCullingCS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", IndexCountPerMeshlet);
            shaderDesc.AddDefine("MESHLET_CULLING", 1);
            shaderLibrary->LoadShader(ShaderID::VirtualGeometryMeshletCulling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/VisibilityBuffer.hslib", "VisibilityBufferVS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", IndexCountPerMeshlet);
            shaderLibrary->LoadShader(ShaderID::VisibilityBufferVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/VisibilityBuffer.hslib", "VisibilityBufferPS");
            shaderLibrary->LoadShader(ShaderID::VisibilityBufferPS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Amplification, "Shaders/RealTimeRenderer/VisibilityBufferMeshShading.hslib", "VisibilityBufferAS");
            //shaderLibrary->LoadShader(ShaderID::VisibilityBufferMeshShadingAS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Mesh, "Shaders/RealTimeRenderer/VisibilityBufferMeshShading.hslib", "VisibilityBufferMS");
            //shaderLibrary->LoadShader(ShaderID::VisibilityBufferMeshShadingMS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/VisibilityBufferMeshShading.hslib", "VisibilityBufferPS");
            //shaderLibrary->LoadShader(ShaderID::VisibilityBufferMeshShadingPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/MotionVectors.hslib", "MotionVectorsCS");
            shaderLibrary->LoadShader(ShaderID::MotionVectors, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/BuildDepthPyramid.hslib", "BuildDepthPyramidCS");
            shaderLibrary->LoadShader(ShaderID::BuildDepthPyramid, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/GBuffer.hslib", "GBufferCS");
            shaderLibrary->LoadShader(ShaderID::GBuffer, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ManyLightRendering/LightGrid/LightGridBufferInitialization.hslib", "LightGridBufferInitializationCS");
            shaderLibrary->LoadShader(ShaderID::LightGridBufferInitialization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ManyLightRendering/LightGrid/LightGridLocalLightCulling.hslib", "LightGridLocalLightCullingCS");
            shaderLibrary->LoadShader(ShaderID::LightGridLocalLightCulling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ManyLightRendering/LightGrid/LightGridDebugVisualization.hslib", "LightGridDebugVisualizationCS");
            shaderLibrary->LoadShader(ShaderID::LightGridDebugVisualization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VirtualShadowMap/VirtualShadowMapClearPageTable.hslib", "VirtualShadowMapClearPageTableCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapClearPageTable, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VirtualShadowMap/VirtualShadowMapPageRequest.hslib", "VirtualShadowMapPageRequestCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapPageRequest, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VirtualShadowMap/VirtualShadowMapPhysicalPageAllocation.hslib", "VirtualShadowMapPhysicalPageAllocationCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapPhysicalPageAllocation, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VirtualShadowMap/VirtualShadowMapClearIndirectArgumentBuffer.hslib", "VirtualShadowMapClearIndirectArgumentBufferCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapClearIndirectArgumentBuffer, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VirtualShadowMap/VirtualShadowMapPhysicalMemoryAllocation.hslib", "VirtualShadowMapPhysicalMemoryAllocationCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapPhysicalMemoryAllocation, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VirtualShadowMap/VirtualShadowMapClearPhysicalMemory.hslib", "VirtualShadowMapClearPhysicalMemoryCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapClearPhysicalMemory, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/VirtualShadowMap/VirtualShadowMapDepth.hslib", "VirtualShadowMapDepthVS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapDepthVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/VirtualShadowMap/VirtualShadowMapDepth.hslib", "VirtualShadowMapDepthPS");
            if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapDepthPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VirtualShadowMap/VirtualShadowMapProjection.hslib", "VirtualShadowMapProjectionCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapProjection, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/CascadedShadowMap.hslib", "CascadedShadowMapVS");
            shaderLibrary->LoadShader(ShaderID::CascadedShadowMapVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/CascadedShadowMap.hslib", "CascadedShadowMapPS");
            shaderLibrary->LoadShader(ShaderID::CascadedShadowMapPS, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/LocalLightShadows.hslib", "LocalLightShadowsVS");
        //     shaderLibrary->LoadShader(ShaderID::LocalLightShadowsVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/LocalLightShadows.hslib", "LocalLightShadowsPS");
        //     shaderLibrary->LoadShader(ShaderID::LocalLightShadowsPS, shaderDesc);
        // }

        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ShadowMapProjection.hslib", "ShadowMapProjectionForDistantLightCS");
            shaderLibrary->LoadShader(ShaderID::ShadowMapProjectionForDistantLight, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceShadows/ScreenSpaceShadowsStochastic.hslib", "ScreenSpaceShadowsStochasticCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceShadowsStochastic, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceShadows/ScreenSpaceShadowsBend.hslib", "ScreenSpaceShadowsBendCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceShadowsBend, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/ScreenSpaceShadows/ScreenSpaceShadowsComposition.hslib", "ScreenSpaceShadowsCompositionPS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceShadowsComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceRayTracing/ScreenSpaceIndirectDiffuse.hslib", "ScreenSpaceIndirectDiffuseCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceIndirectDiffuse, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceRayTracing/ScreenSpaceReflectionsTileClassification.hslib", "SSRTileClassificationHorizontalCS");
            shaderDesc.AddDefine("SSR_TILE_CLASSIFICATION_HORIZONTAL_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SSRTileClassificationHorizontal, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceRayTracing/ScreenSpaceReflectionsTileClassification.hslib", "SSRTileClassificationVerticalCS");
            shaderDesc.AddDefine("SSR_TILE_CLASSIFICATION_VERTICAL_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SSRTileClassificationVertical, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceRayTracing/ScreenSpaceReflectionsRayAllocation.hslib", "SSRRayAllocationCS");
            shaderLibrary->LoadShader(ShaderID::SSRRayAllocation, shaderDesc);
        }
        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflections.hslib", "ScreenSpaceReflectionsCS");
        //    shaderDesc.AddDefine(HE_TEXT("SSR_EARLY_EXIT_RAYS"));
        //    shaderLibrary->LoadShader(ShaderID::SSRRayTracingEarlyExit, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflections.hslib", "ScreenSpaceReflectionsCS");
        //    shaderDesc.AddDefine(HE_TEXT("SSR_CHEAP_RAYS"));
        //    shaderLibrary->LoadShader(ShaderID::SSRRayTracingCheap, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflections.hslib", "ScreenSpaceReflectionsCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRRayTracingExpensive, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflectionsResolve.hslib", "ScreenSpaceReflectionsResolveCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRColorResolve, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflectionsTemporalFiltering.hslib", "SSRTemporalFilteringCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRTemporalFiltering, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceReflectionsSpatialFiltering.hslib", "SSRSpatialFilteringCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRSpatialFiltering, shaderDesc);
        //}
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/GTAOHorizonSearchAndIntegral.hslib", "GTAOHorizonSearchAndIntegralCS");
        //     shaderLibrary->LoadShader(ShaderID::GTAOHorizonSearchAndIntegral, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/GTAOSpatialFiltering.hslib", "GTAOSpatialFilteringCS");
        //     shaderLibrary->LoadShader(ShaderID::GTAOSpatialFiltering, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceAmbientOcclusion.hslib", "GTAOTemporalFilteringCS");
        //     shaderLibrary->LoadShader(ShaderID::GTAOTemporalFiltering, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/IndirectDiffuseComposition.hslib", "IndirectDiffuseCompositionPS");
            shaderLibrary->LoadShader(ShaderID::IndirectDiffuseComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/IndirectSpecularComposition.hslib", "IndirectSpecularCompositionPS");
            shaderLibrary->LoadShader(ShaderID::IndirectSpecularComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/DirectLighting.hslib", "DirectLightingPS");
            if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderLibrary->LoadShader(ShaderID::DirectLighting, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/SkyBox.hslib", "SkyBoxVS");
        //     shaderLibrary->LoadShader(ShaderID::SkyBoxVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/SkyBox.hslib", "SkyBoxPS");
        //     shaderLibrary->LoadShader(ShaderID::SkyBoxPS, shaderDesc);
        // }
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringInitialize.hslib", "SubsurfaceScatteringInitializeCS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringInitialize, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringClassifyTiles.hslib", "SubsurfaceScatteringClassifyTilesCS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringClassifyTiles, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringBuildIndirectArguments.hslib", "SubsurfaceScatteringBuildIndirectArgumentsCS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringBuildIndirectArguments, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringSampleDiffusionProfile.hslib", "SubsurfaceScatteringSampleDiffusionProfileCS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringSampleDiffusionProfile, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringComputeVariance.hslib", "SubsurfaceScatteringComputeVarianceCS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringComputeVariance, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringCopyResults.hslib", "SubsurfaceScatteringCopyResultsVS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringCopyResultsVS, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringCopyResults.hslib", "SubsurfaceScatteringCopyResultsPS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringCopyResultsPS, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringRecombine.hslib", "SubsurfaceScatteringRecombineVS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringRecombineVS, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/SubsurfaceScattering/SubsurfaceScatteringRecombine.hslib", "SubsurfaceScatteringRecombinePS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringRecombinePS, shaderDesc);
        //}
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIFreeSurfels.hslib", "SurfelGIFreeSurfelsCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIFreeSurfels, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIGapFilling.hslib", "SurfelGIGapFillingCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIGapFilling, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIIndirectArguments.hslib", "SurfelGIIndirectArgumentsCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIIndirectArguments, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIGridReset.hslib", "SurfelGIGridResetCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIGridReset, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIComputeCellCapacity.hslib", "SurfelGIComputeCellCapacityCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIComputeCellCapacity, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIComputeCellOffset.hslib", "SurfelGIComputeCellOffsetCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIComputeCellOffset, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/SurfelGI/SurfelGIVisualization.hslib", "SurfelGIVisualizationCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIVisualization, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceLightShafts/ScreenSpaceLightShaftsDownsample.hslib", "ScreenSpaceLightShaftsDownsampleCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceLightShaftsDownsample, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceLightShafts/ScreenSpaceLightShaftsTemporalFiltering.hslib", "ScreenSpaceLightShaftsTemporalFilteringCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceLightShaftsTemporalFiltering, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/ScreenSpaceLightShafts/ScreenSpaceLightShaftsRadialBlur.hslib", "ScreenSpaceLightShaftsRadialBlurCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceLightShaftsRadialBlur, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/ScreenSpaceLightShafts/ScreenSpaceLightShaftsComposition.hslib", "ScreenSpaceLightShaftsCompositionPS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceLightShaftsComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VolumetricFog/VolumetricFogVoxelization.hslib", "VolumetricFogVoxelizationCS");
            shaderLibrary->LoadShader(ShaderID::VolumetricFogVoxelization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VolumetricFog/VolumetricFogLightScattering.hslib", "VolumetricFogLightScatteringCS");
            shaderLibrary->LoadShader(ShaderID::VolumetricFogLightScattering, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/VolumetricFog/VolumetricFogFinalIntegration.hslib", "VolumetricFogFinalIntegrationCS");
            shaderLibrary->LoadShader(ShaderID::VolumetricFogFinalIntegration, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/VolumetricFog/VolumetricFogComposition.hslib", "VolumetricFogCompositionPS");
            shaderLibrary->LoadShader(ShaderID::VolumetricFogComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/VolumetricFog/LocalFogVolume.hslib", "LocalFogVolumeVS");
            shaderLibrary->LoadShader(ShaderID::LocalFogVolumeVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/VolumetricFog/LocalFogVolume.hslib", "LocalFogVolumePS");
            shaderLibrary->LoadShader(ShaderID::LocalFogVolumePS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/MotionBlur/MotionBlurTileClassification.hslib", "MotionBlurTileClassificationCS");
            shaderLibrary->LoadShader(ShaderID::MotionBlurTileClassificationCS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/PostProcessing/MotionBlur/MotionBlurVelocityDilation.hslib", "MotionBlurVelocityDilationScatterVS");
            shaderLibrary->LoadShader(ShaderID::MotionBlurVelocityDilationScatterVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/PostProcessing/MotionBlur/MotionBlurVelocityDilation.hslib", "MotionBlurVelocityDilationScatterPS");
            shaderLibrary->LoadShader(ShaderID::MotionBlurVelocityDilationScatterPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/MotionBlur/MotionBlurReconstructionFilter.hslib", "MotionBlurReconstructionFilterCS");
            shaderLibrary->LoadShader(ShaderID::MotionBlurReconstructionFilterCS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/TemporalSuperSampling/TemporalSuperSampling.hslib", "TemporalSuperSamplingCS");
            //shaderLibrary->LoadShader(ShaderID::TemporalSuperSampling, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/DepthOfFieldSetup.hslib", "DepthOfFieldSetupCS");
            //shaderLibrary->LoadShader(ShaderID::DepthOfFieldSetup, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/DepthOfFieldGather.hslib", "DepthOfFieldGatherCS");
            //shaderLibrary->LoadShader(ShaderID::DepthOfFieldGather, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/DepthOfFieldPostfilter.hslib", "DepthOfFieldPostfilterCS");
            //shaderLibrary->LoadShader(ShaderID::DepthOfFieldPostfilter, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/DepthOfFieldRecombine.hslib", "DepthOfFieldRecombineCS");
            //shaderLibrary->LoadShader(ShaderID::DepthOfFieldRecombine, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/AutoExposureBuildHistogram.hslib", "AutoExposureBuildHistogramCS");
            if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderLibrary->LoadShader(ShaderID::AutoExposureBuildHistogram, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/AutoExposureComputeExposure.hslib", "AutoExposureComputeExposureCS");
            shaderLibrary->LoadShader(ShaderID::AutoExposureComputeExposure, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/CopyExposure.hslib", "CopyExposureCS");
            shaderLibrary->LoadShader(ShaderID::CopyExposure, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/BuildColorPyramid.hslib", "BuildColorPyramidCS");
            shaderLibrary->LoadShader(ShaderID::BuildColorPyramid, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/GaussianBloomDownsample.hslib", "GaussianBloomDownsampleCS");
            shaderLibrary->LoadShader(ShaderID::GaussianBloomDownsample, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/GaussianBloomUpsample.hslib", "GaussianBloomUpsampleCS");
            shaderLibrary->LoadShader(ShaderID::GaussianBloomUpsample, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/ConvolutionBloomResizeKernel.hslib", "ConvolutionBloomResizeKernelCS");
            shaderLibrary->LoadShader(ShaderID::ConvolutionBloomResizeKernel, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/PostProcessing/LensFlareGhost.hslib", "LensFlareGhostPS");
            shaderLibrary->LoadShader(ShaderID::LensFlareGhost, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LensFlareTileCulling.hslib", "LensFlareTileCullingCS");
        //     shaderLibrary->LoadShader(ShaderID::LensFlareTileCulling, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/PostProcessing/LensFlareGlare.hslib", "LensFlareGlareVS");
        //     shaderLibrary->LoadShader(ShaderID::LensFlareGlareVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/PostProcessing/LensFlareGlare.hslib", "LensFlareGlarePS");
        //     shaderLibrary->LoadShader(ShaderID::LensFlareGlarePS, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/PostProcessing/LensFlareCombine.hslib", "LensFlareCombinePS");
            shaderLibrary->LoadShader(ShaderID::LensFlareCombine, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingBuildGrid.hslib", "BilateralGridLocalToneMappingBuildGridCS");
            shaderLibrary->LoadShader(ShaderID::BilateralGridLocalToneMappingBuildGrid, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingComputeLogLuminance.hslib", "BilateralGridLocalToneMappingComputeLogLuminanceCS");
            shaderLibrary->LoadShader(ShaderID::BilateralGridLocalToneMappingComputeLogLuminance, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingGaussianDistribution.hslib", "BilateralGridLocalToneMappingGaussianDistributionCS");
            shaderLibrary->LoadShader(ShaderID::BilateralGridLocalToneMappingGaussianDistribution, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingGaussianFilter.hslib", "BilateralGridLocalToneMappingGaussianFilterCS");
            shaderLibrary->LoadShader(ShaderID::BilateralGridLocalToneMappingGaussianFilter, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingUpsampling.hslib", "BilateralGridLocalToneMappingUpsamplingCS");
            shaderLibrary->LoadShader(ShaderID::BilateralGridLocalToneMappingUpsampling, shaderDesc);
        }
         {
             ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalToneMapping/ExposureFusionComputeLuminanceAndWeight.hslib", "ExposureFusionComputeLuminanceAndWeightCS");
             shaderLibrary->LoadShader(ShaderID::ExposureFusionComputeLuminanceAndWeight, shaderDesc);
         }
         {
             ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalToneMapping/LocalToneMappingBlendExposures.hslib", "LocalToneMappingBlendExposuresCS");
             shaderLibrary->LoadShader(ShaderID::LocalToneMappingBlendExposures, shaderDesc);
         }
         {
             ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalToneMapping/LocalToneMappingBlendLaplacian.hslib", "LocalToneMappingBlendLaplacianCS");
             shaderLibrary->LoadShader(ShaderID::LocalToneMappingBlendLaplacian, shaderDesc);
         }
         {
             ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/LocalToneMapping/ExposureFusionGuidedUpsampling.hslib", "ExposureFusionGuidedUpsamplingCS");
             shaderLibrary->LoadShader(ShaderID::ExposureFusionGuidedUpsampling, shaderDesc);
         }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/ColorTransformLUT.hslib", "ColorTransformLUTCS");
            if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderLibrary->LoadShader(ShaderID::ColorTransformLUT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/FinalComposition.hslib", "FinalCompositionCS");
            shaderLibrary->LoadShader(ShaderID::FinalComposition, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/PostProcessing/SelectionOutlineMask.hslib", "SelectionOutlineMaskVS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineMaskVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/PostProcessing/SelectionOutlineMask.hslib", "SelectionOutlineMaskPS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineMaskPS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/SelectionOutlineSetup.hslib", "SelectionOutlineSetupCS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineSetup, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/SelectionOutlineJumpFlood.hslib", "SelectionOutlineJumpFloodCS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineJumpFlood, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/PostProcessing/SelectionOutlineComposite.hslib", "SelectionOutlineCompositeCS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineComposite, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeDepth.hslib", "VisualizeDepthCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeDepth, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VirtualGeometryDebugVisualization.hslib", "VirtualGeometryDebugVisualizationCS");
            if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderLibrary->LoadShader(ShaderID::VirtualGeometryDebugVisualization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeWorldSpaceNormal.hslib", "VisualizeWorldSpaceNormalCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeWorldSpaceNormal, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeMotionVectors.hslib", "VisualizeMotionVectorsCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeMotionVectors, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeAmbientOcclusion.hslib", "VisualizeAmbientOcclusionCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeAmbientOcclusion, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeScreenSpaceShadowMask.hslib", "VisualizeScreenSpaceShadowMaskCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeScreenSpaceShadowMask, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeCascadedShadowMap.hslib", "VisualizeCascadedShadowMapCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeCascadedShadowMap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/DebugVisualization/VisualizeVirtualShadowMap.hslib", "VisualizeVirtualShadowMapCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeVirtualShadowMap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/GUIComposition.hslib", "GUICompositionPS");
            shaderLibrary->LoadShader(ShaderID::GUICompositionPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RealTimeRenderer/DebugVisualization/DebugDrawLines.hslib", "DebugDrawLinesVS");
            shaderLibrary->LoadShader(ShaderID::DebugDrawVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RealTimeRenderer/DebugVisualization/DebugDrawLines.hslib", "DebugDrawLinesPS");
            shaderLibrary->LoadShader(ShaderID::DebugDrawPS, shaderDesc);
        }

        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::RayGen, "Shaders/RealTimeRenderer/HardwareRayTracing/RayTracingShadows.hslib", "RayTracingShadowsRayGen");
            shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
            //if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            //{
            //    shaderDesc.shaderCompilerOptions.skipOptimization = false;
            //    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            //}
            shaderLibrary->LoadShader(ShaderID::RayTracingShadowsRayGen, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Miss, "Shaders/RealTimeRenderer/HardwareRayTracing/RayTracingShadows.hslib", "RayTracingShadowsMiss");
            shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
            if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderLibrary->LoadShader(ShaderID::RayTracingShadowsMiss, shaderDesc);
        }

        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RealTimeRenderer/HardwareRayTracing/RayTracingShadows.hslib", "RayTracingShadowsInlineRayTracingCS");
            shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
            //shaderDesc.shaderCompilerOptions.inlineRayTracing = true;
            //if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderLibrary->LoadShader(ShaderID::RayTracingShadowsInlineRayTracing, shaderDesc);
        }

        {
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::RayGen, "Shaders/PathTracingRenderer/PathTracing.hslib", "PathTracingRayGen");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                {
                    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                }
                shaderLibrary->LoadShader(ShaderID::PathTracingRayGen, shaderDesc);
            }
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Miss, "Shaders/PathTracingRenderer/PathTracing.hslib", "PathTracingDefaultMiss");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                {
                    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                }
                shaderLibrary->LoadShader(ShaderID::PathTracingDefaultMiss, shaderDesc);
            }
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Miss, "Shaders/PathTracingRenderer/PathTracing.hslib", "PathTracingShadowRayMiss");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                {
                    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                }
                shaderLibrary->LoadShader(ShaderID::PathTracingShadowRayMiss, shaderDesc);
            }
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::ClosestHit, "Shaders/PathTracingRenderer/PathTracing.hslib", "PathTracingDefaultOpaqueClosestHit");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                {
                    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                }
                shaderLibrary->LoadShader(ShaderID::PathTracingDefaultOpaqueClosestHit, shaderDesc);
            }
        }
    }
}