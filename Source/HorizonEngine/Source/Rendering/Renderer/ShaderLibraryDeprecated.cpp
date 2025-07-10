#include "ShaderCollection.h"
#include "RasterizationRenderer/RasterizationRenderer.h"

namespace Horizon
{
    static void LoadSkyAtmosphereShaders_Deprecated(ShaderCollection* shaderLibrary)
    {
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SkyAtmosphere/SkyAtmosphereTransmittanceLut.hslib", "SkyAtmosphereTransmittanceLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_TRANSMITTANCE_LUT_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereTransmittanceLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SkyAtmosphere/SkyAtmosphereMultipleScatteringLut.hslib", "SkyAtmosphereMultipleScatteringLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_MULTIPLE_SCATTERING_LUT_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereMultipleScatteringLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SkyAtmosphere/SkyAtmosphereSkyViewLut.hslib", "SkyAtmosphereSkyViewLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_SKY_VIEW_LUT_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereSkyViewLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SkyAtmosphere/SkyAtmosphereAerialPerspectiveVolume.hslib", "SkyAtmosphereAerialPerspectiveVolumeCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_AERIAL_PERSPECTIVE_VOLUME_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SkyAtmosphereAerialPerspectiveVolume, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/SkyAtmosphere/SkyAtmosphereRayMarching.hslib", "SkyAtmosphereRayMarchingPS");
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
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ImageBasedLighting/EnvironmentBRDFIntegration.hslib", "EnvironmentBRDFIntegrationCS");
            shaderLibrary->LoadShader(ShaderID::EnvironmentBRDFIntegration, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ImageBasedLighting/EnvironmentMapConvolution.hslib", "EnvironmentMapConvolutionCS");
            shaderLibrary->LoadShader(ShaderID::EnvironmentMapConvolution, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ImageBasedLighting/IrradianceEnvironmentMapReference.hslib", "IrradianceEnvironmentMapReferenceCS");
            shaderLibrary->LoadShader(ShaderID::IrradianceEnvironmentMapReference, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ImageBasedLighting/IrradianceEnvironmentMapSHOnePass.hslib", "IrradianceEnvironmentMapSHOnePassCS");
            shaderLibrary->LoadShader(ShaderID::IrradianceEnvironmentMapSHOnePass, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ImageBasedLighting/IrradianceEnvironmentMapSHSampling.hslib", "IrradianceEnvironmentMapSHSamplingCS");
        //     shaderLibrary->LoadShader(ShaderID::IrradianceEnvironmentMapSHSampling, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ImageBasedLighting/IrradianceEnvironmentMapSHIntegration.hslib", "IrradianceEnvironmentMapSHIntegrationCS");
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
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VisibilityCulling.hslib", "IndirectArgumentInitializationCS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", IndexCountPerMeshlet);
            shaderDesc.AddDefine("INDIRECT_ARGUMENT_INITIALIZATION", 1);
            shaderLibrary->LoadShader(ShaderID::VisibilityCullingIndirectArgumentInitialization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VisibilityCulling.hslib", "InstanceCullingCS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", IndexCountPerMeshlet);
            shaderDesc.AddDefine("INSTANCE_CULLING", 1);
            shaderLibrary->LoadShader(ShaderID::VirtualGeometryInstanceCulling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VisibilityCulling.hslib", "MeshletCullingCS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", IndexCountPerMeshlet);
            shaderDesc.AddDefine("MESHLET_CULLING", 1);
            shaderLibrary->LoadShader(ShaderID::VirtualGeometryMeshletCulling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/VisibilityBuffer.hslib", "VisibilityBufferVS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", IndexCountPerMeshlet);
            shaderLibrary->LoadShader(ShaderID::VisibilityBufferVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/VisibilityBuffer.hslib", "VisibilityBufferPS");
            shaderLibrary->LoadShader(ShaderID::VisibilityBufferPS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Amplification, "Shaders/RasterizationRenderer/VisibilityBufferMeshShading.hslib", "VisibilityBufferAS");
            //shaderLibrary->LoadShader(ShaderID::VisibilityBufferMeshShadingAS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Mesh, "Shaders/RasterizationRenderer/VisibilityBufferMeshShading.hslib", "VisibilityBufferMS");
            //shaderLibrary->LoadShader(ShaderID::VisibilityBufferMeshShadingMS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/VisibilityBufferMeshShading.hslib", "VisibilityBufferPS");
            //shaderLibrary->LoadShader(ShaderID::VisibilityBufferMeshShadingPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/MotionVectors.hslib", "MotionVectorsCS");
            shaderLibrary->LoadShader(ShaderID::MotionVectors, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/BuildDepthPyramid.hslib", "BuildDepthPyramidCS");
            shaderLibrary->LoadShader(ShaderID::BuildDepthPyramid, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/GBuffer.hslib", "GBufferCS");
            shaderLibrary->LoadShader(ShaderID::GBuffer, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ManyLightRendering/LightGrid/LightGridBufferInitialization.hslib", "LightGridBufferInitializationCS");
            shaderLibrary->LoadShader(ShaderID::LightGridBufferInitialization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ManyLightRendering/LightGrid/LightGridLocalLightCulling.hslib", "LightGridLocalLightCullingCS");
            shaderLibrary->LoadShader(ShaderID::LightGridLocalLightCulling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ManyLightRendering/LightGrid/LightGridDebugVisualization.hslib", "LightGridDebugVisualizationCS");
            shaderLibrary->LoadShader(ShaderID::LightGridDebugVisualization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapClearPageTable.hslib", "VirtualShadowMapClearPageTableCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapClearPageTable, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapPageRequest.hslib", "VirtualShadowMapPageRequestCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapPageRequest, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapPhysicalPageAllocation.hslib", "VirtualShadowMapPhysicalPageAllocationCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapPhysicalPageAllocation, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapClearIndirectArgumentBuffer.hslib", "VirtualShadowMapClearIndirectArgumentBufferCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapClearIndirectArgumentBuffer, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapPhysicalMemoryAllocation.hslib", "VirtualShadowMapPhysicalMemoryAllocationCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapPhysicalMemoryAllocation, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapClearPhysicalMemory.hslib", "VirtualShadowMapClearPhysicalMemoryCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapClearPhysicalMemory, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapDepth.hslib", "VirtualShadowMapDepthVS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapDepthVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapDepth.hslib", "VirtualShadowMapDepthPS");
            if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapDepthPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapProjection.hslib", "VirtualShadowMapProjectionCS");
            shaderLibrary->LoadShader(ShaderID::VirtualShadowMapProjection, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/CascadedShadowMap.hslib", "CascadedShadowMapVS");
            shaderLibrary->LoadShader(ShaderID::CascadedShadowMapVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/CascadedShadowMap.hslib", "CascadedShadowMapPS");
            shaderLibrary->LoadShader(ShaderID::CascadedShadowMapPS, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/LocalLightShadows.hslib", "LocalLightShadowsVS");
        //     shaderLibrary->LoadShader(ShaderID::LocalLightShadowsVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/LocalLightShadows.hslib", "LocalLightShadowsPS");
        //     shaderLibrary->LoadShader(ShaderID::LocalLightShadowsPS, shaderDesc);
        // }

        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ShadowMapProjection.hslib", "ShadowMapProjectionForDistantLightCS");
            shaderLibrary->LoadShader(ShaderID::ShadowMapProjectionForDistantLight, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceShadows/ScreenSpaceShadowsStochastic.hslib", "ScreenSpaceShadowsStochasticCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceShadowsStochastic, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceShadows/ScreenSpaceShadowsBend.hslib", "ScreenSpaceShadowsBendCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceShadowsBend, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/ScreenSpaceShadows/ScreenSpaceShadowsComposition.hslib", "ScreenSpaceShadowsCompositionPS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceShadowsComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceRayTracing/ScreenSpaceIndirectDiffuse.hslib", "ScreenSpaceIndirectDiffuseCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceIndirectDiffuse, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceRayTracing/ScreenSpaceReflectionsTileClassification.hslib", "SSRTileClassificationHorizontalCS");
            shaderDesc.AddDefine("SSR_TILE_CLASSIFICATION_HORIZONTAL_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SSRTileClassificationHorizontal, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceRayTracing/ScreenSpaceReflectionsTileClassification.hslib", "SSRTileClassificationVerticalCS");
            shaderDesc.AddDefine("SSR_TILE_CLASSIFICATION_VERTICAL_PASS", 1);
            shaderLibrary->LoadShader(ShaderID::SSRTileClassificationVertical, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceRayTracing/ScreenSpaceReflectionsRayAllocation.hslib", "SSRRayAllocationCS");
            shaderLibrary->LoadShader(ShaderID::SSRRayAllocation, shaderDesc);
        }
        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceReflections.hslib", "ScreenSpaceReflectionsCS");
        //    shaderDesc.AddDefine(HE_TEXT("SSR_EARLY_EXIT_RAYS"));
        //    shaderLibrary->LoadShader(ShaderID::SSRRayTracingEarlyExit, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceReflections.hslib", "ScreenSpaceReflectionsCS");
        //    shaderDesc.AddDefine(HE_TEXT("SSR_CHEAP_RAYS"));
        //    shaderLibrary->LoadShader(ShaderID::SSRRayTracingCheap, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceReflections.hslib", "ScreenSpaceReflectionsCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRRayTracingExpensive, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceReflectionsResolve.hslib", "ScreenSpaceReflectionsResolveCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRColorResolve, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceReflectionsTemporalFiltering.hslib", "SSRTemporalFilteringCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRTemporalFiltering, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceReflectionsSpatialFiltering.hslib", "SSRSpatialFilteringCS");
        //    shaderLibrary->LoadShader(ShaderID::SSRSpatialFiltering, shaderDesc);
        //}
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceAmbientOcclusion.hslib", "GTAOHorizonSearchIntegralCS");
            shaderDesc.AddDefine("GTAO_HORIZON_SEARCH_INTEGRAL", 1);
            shaderLibrary->LoadShader(ShaderID::GTAOHorizonSearchIntegral, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceAmbientOcclusion.hslib", "GTAOSpatialFilteringCS");
            shaderDesc.AddDefine("GTAO_SPATIAL_FILTERING", 1);
            shaderLibrary->LoadShader(ShaderID::GTAOSpatialFiltering, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceAmbientOcclusion.hslib", "GTAOTemporalFilteringCS");
            shaderDesc.AddDefine("GTAO_TEMPORAL_FILTERING", 1);
            shaderLibrary->LoadShader(ShaderID::GTAOTemporalFiltering, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceAmbientOcclusion.hslib", "GTAOUpsamplingCS");
            shaderDesc.AddDefine("GTAO_UPSAMPLING", 1);
            shaderLibrary->LoadShader(ShaderID::GTAOUpsampling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/IndirectDiffuseComposition.hslib", "IndirectDiffuseCompositionPS");
            shaderLibrary->LoadShader(ShaderID::IndirectDiffuseComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/IndirectSpecularComposition.hslib", "IndirectSpecularCompositionPS");
            shaderLibrary->LoadShader(ShaderID::IndirectSpecularComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/DirectLighting.hslib", "DirectLightingPS");
            if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderLibrary->LoadShader(ShaderID::DirectLighting, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/SkyBox.hslib", "SkyBoxVS");
        //     shaderLibrary->LoadShader(ShaderID::SkyBoxVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/SkyBox.hslib", "SkyBoxPS");
        //     shaderLibrary->LoadShader(ShaderID::SkyBoxPS, shaderDesc);
        // }
        {
           ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringInitialization.hslib", "SubsurfaceScatteringInitializationCS");
           shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringInitialization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringTileClassification.hslib", "SubsurfaceScatteringTileClassificationCS");
            shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringTileClassification, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringBuildIndirectArguments.hslib", "SubsurfaceScatteringBuildIndirectArgumentsCS");
            shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringBuildIndirectArguments, shaderDesc);
        }
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringSampleDiffusionProfile.hslib", "SubsurfaceScatteringSampleDiffusionProfileCS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringSampleDiffusionProfile, shaderDesc);
        //}
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringComputeVariance.hslib", "SubsurfaceScatteringComputeVarianceCS");
        //    shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringComputeVariance, shaderDesc);
        //}
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringLightingComposition.hslib", "SubsurfaceScatteringLightingCompositionVS");
            shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringLightingCompositionVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringLightingComposition.hslib", "SubsurfaceScatteringLightingCompositionPS");
            shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringLightingCompositionPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringCopyResults.hslib", "SubsurfaceScatteringCopyResultsVS");
            shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringCopyResultsVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringCopyResults.hslib", "SubsurfaceScatteringCopyResultsPS");
            shaderLibrary->LoadShader(ShaderID::SubsurfaceScatteringCopyResultsPS, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIFreeSurfels.hslib", "SurfelGIFreeSurfelsCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIFreeSurfels, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIGapFilling.hslib", "SurfelGIGapFillingCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIGapFilling, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIIndirectArguments.hslib", "SurfelGIIndirectArgumentsCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIIndirectArguments, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIGridReset.hslib", "SurfelGIGridResetCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIGridReset, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIComputeCellCapacity.hslib", "SurfelGIComputeCellCapacityCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIComputeCellCapacity, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIComputeCellOffset.hslib", "SurfelGIComputeCellOffsetCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIComputeCellOffset, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIVisualization.hslib", "SurfelGIVisualizationCS");
        //     shaderLibrary->LoadShader(ShaderID::SurfelGIVisualization, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceLightShafts/ScreenSpaceLightShaftsDownsample.hslib", "ScreenSpaceLightShaftsDownsampleCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceLightShaftsDownsample, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceLightShafts/ScreenSpaceLightShaftsTemporalFiltering.hslib", "ScreenSpaceLightShaftsTemporalFilteringCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceLightShaftsTemporalFiltering, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceLightShafts/ScreenSpaceLightShaftsRadialBlur.hslib", "ScreenSpaceLightShaftsRadialBlurCS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceLightShaftsRadialBlur, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/ScreenSpaceLightShafts/ScreenSpaceLightShaftsComposition.hslib", "ScreenSpaceLightShaftsCompositionPS");
            shaderLibrary->LoadShader(ShaderID::ScreenSpaceLightShaftsComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VolumetricFog/VolumetricFogVoxelization.hslib", "VolumetricFogVoxelizationCS");
            shaderLibrary->LoadShader(ShaderID::VolumetricFogVoxelization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VolumetricFog/VolumetricFogLightScattering.hslib", "VolumetricFogLightScatteringCS");
            shaderLibrary->LoadShader(ShaderID::VolumetricFogLightScattering, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VolumetricFog/VolumetricFogFinalIntegration.hslib", "VolumetricFogFinalIntegrationCS");
            shaderLibrary->LoadShader(ShaderID::VolumetricFogFinalIntegration, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/VolumetricFog/VolumetricFogComposition.hslib", "VolumetricFogCompositionPS");
            shaderLibrary->LoadShader(ShaderID::VolumetricFogComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/VolumetricFog/LocalFogVolume.hslib", "LocalFogVolumeVS");
            shaderLibrary->LoadShader(ShaderID::LocalFogVolumeVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/VolumetricFog/LocalFogVolume.hslib", "LocalFogVolumePS");
            shaderLibrary->LoadShader(ShaderID::LocalFogVolumePS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/MotionBlur/MotionBlurTileClassification.hslib", "MotionBlurTileClassificationCS");
            shaderLibrary->LoadShader(ShaderID::MotionBlurTileClassificationCS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/PostProcessing/MotionBlur/MotionBlurVelocityDilation.hslib", "MotionBlurVelocityDilationScatterVS");
            shaderLibrary->LoadShader(ShaderID::MotionBlurVelocityDilationScatterVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/PostProcessing/MotionBlur/MotionBlurVelocityDilation.hslib", "MotionBlurVelocityDilationScatterPS");
            shaderLibrary->LoadShader(ShaderID::MotionBlurVelocityDilationScatterPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/MotionBlur/MotionBlurReconstructionFilter.hslib", "MotionBlurReconstructionFilterCS");
            shaderLibrary->LoadShader(ShaderID::MotionBlurReconstructionFilterCS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/TemporalSuperSampling/TemporalSuperSampling.hslib", "TemporalSuperSamplingCS");
            //shaderLibrary->LoadShader(ShaderID::TemporalSuperSampling, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/DepthOfFieldSetup.hslib", "DepthOfFieldSetupCS");
            //shaderLibrary->LoadShader(ShaderID::DepthOfFieldSetup, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/DepthOfFieldGather.hslib", "DepthOfFieldGatherCS");
            //shaderLibrary->LoadShader(ShaderID::DepthOfFieldGather, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/DepthOfFieldPostfilter.hslib", "DepthOfFieldPostfilterCS");
            //shaderLibrary->LoadShader(ShaderID::DepthOfFieldPostfilter, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/DepthOfFieldRecombine.hslib", "DepthOfFieldRecombineCS");
            //shaderLibrary->LoadShader(ShaderID::DepthOfFieldRecombine, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/AutoExposureBuildHistogram.hslib", "AutoExposureBuildHistogramCS");
            if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderLibrary->LoadShader(ShaderID::AutoExposureBuildHistogram, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/AutoExposureComputeExposure.hslib", "AutoExposureComputeExposureCS");
            shaderLibrary->LoadShader(ShaderID::AutoExposureComputeExposure, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/CopyExposure.hslib", "CopyExposureCS");
            shaderLibrary->LoadShader(ShaderID::CopyExposure, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/BuildColorPyramid.hslib", "BuildColorPyramidCS");
            shaderLibrary->LoadShader(ShaderID::BuildColorPyramid, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/GaussianBloomDownsample.hslib", "GaussianBloomDownsampleCS");
            shaderLibrary->LoadShader(ShaderID::GaussianBloomDownsample, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/GaussianBloomUpsample.hslib", "GaussianBloomUpsampleCS");
            shaderLibrary->LoadShader(ShaderID::GaussianBloomUpsample, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/ConvolutionBloomResizeKernel.hslib", "ConvolutionBloomResizeKernelCS");
            shaderLibrary->LoadShader(ShaderID::ConvolutionBloomResizeKernel, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/PostProcessing/LensFlareGhost.hslib", "LensFlareGhostPS");
            shaderLibrary->LoadShader(ShaderID::LensFlareGhost, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LensFlareTileCulling.hslib", "LensFlareTileCullingCS");
        //     shaderLibrary->LoadShader(ShaderID::LensFlareTileCulling, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/PostProcessing/LensFlareGlare.hslib", "LensFlareGlareVS");
        //     shaderLibrary->LoadShader(ShaderID::LensFlareGlareVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/PostProcessing/LensFlareGlare.hslib", "LensFlareGlarePS");
        //     shaderLibrary->LoadShader(ShaderID::LensFlareGlarePS, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/PostProcessing/LensFlareCombine.hslib", "LensFlareCombinePS");
            shaderLibrary->LoadShader(ShaderID::LensFlareCombine, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingBuildGrid.hslib", "BilateralGridLocalToneMappingBuildGridCS");
            shaderLibrary->LoadShader(ShaderID::BilateralGridLocalToneMappingBuildGrid, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingComputeLogLuminance.hslib", "BilateralGridLocalToneMappingComputeLogLuminanceCS");
            shaderLibrary->LoadShader(ShaderID::BilateralGridLocalToneMappingComputeLogLuminance, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingGaussianDistribution.hslib", "BilateralGridLocalToneMappingGaussianDistributionCS");
            shaderLibrary->LoadShader(ShaderID::BilateralGridLocalToneMappingGaussianDistribution, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingGaussianFilter.hslib", "BilateralGridLocalToneMappingGaussianFilterCS");
            shaderLibrary->LoadShader(ShaderID::BilateralGridLocalToneMappingGaussianFilter, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingUpsampling.hslib", "BilateralGridLocalToneMappingUpsamplingCS");
            shaderLibrary->LoadShader(ShaderID::BilateralGridLocalToneMappingUpsampling, shaderDesc);
        }
         {
             ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/ExposureFusionComputeLuminanceAndWeight.hslib", "ExposureFusionComputeLuminanceAndWeightCS");
             shaderLibrary->LoadShader(ShaderID::ExposureFusionComputeLuminanceAndWeight, shaderDesc);
         }
         {
             ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/LocalToneMappingBlendExposures.hslib", "LocalToneMappingBlendExposuresCS");
             shaderLibrary->LoadShader(ShaderID::LocalToneMappingBlendExposures, shaderDesc);
         }
         {
             ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/LocalToneMappingBlendLaplacian.hslib", "LocalToneMappingBlendLaplacianCS");
             shaderLibrary->LoadShader(ShaderID::LocalToneMappingBlendLaplacian, shaderDesc);
         }
         {
             ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/ExposureFusionGuidedUpsampling.hslib", "ExposureFusionGuidedUpsamplingCS");
             shaderLibrary->LoadShader(ShaderID::ExposureFusionGuidedUpsampling, shaderDesc);
         }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/ColorTransformLUT.hslib", "ColorTransformLUTCS");
            if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderLibrary->LoadShader(ShaderID::ColorTransformLUT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/FinalComposition.hslib", "FinalCompositionCS");
            shaderLibrary->LoadShader(ShaderID::FinalComposition, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/PostProcessing/SelectionOutlineMask.hslib", "SelectionOutlineMaskVS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineMaskVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/PostProcessing/SelectionOutlineMask.hslib", "SelectionOutlineMaskPS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineMaskPS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/SelectionOutlineSetup.hslib", "SelectionOutlineSetupCS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineSetup, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/SelectionOutlineJumpFlood.hslib", "SelectionOutlineJumpFloodCS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineJumpFlood, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/SelectionOutlineComposite.hslib", "SelectionOutlineCompositeCS");
        //     shaderLibrary->LoadShader(ShaderID::SelectionOutlineComposite, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeDepth.hslib", "VisualizeDepthCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeDepth, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VirtualGeometryDebugVisualization.hslib", "VirtualGeometryDebugVisualizationCS");
            if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderLibrary->LoadShader(ShaderID::VirtualGeometryDebugVisualization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeWorldSpaceNormal.hslib", "VisualizeWorldSpaceNormalCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeWorldSpaceNormal, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeMotionVectors.hslib", "VisualizeMotionVectorsCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeMotionVectors, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeAmbientOcclusion.hslib", "VisualizeAmbientOcclusionCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeAmbientOcclusion, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeScreenSpaceShadowMask.hslib", "VisualizeScreenSpaceShadowMaskCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeScreenSpaceShadowMask, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeCascadedShadowMap.hslib", "VisualizeCascadedShadowMapCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeCascadedShadowMap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeVirtualShadowMap.hslib", "VisualizeVirtualShadowMapCS");
            shaderLibrary->LoadShader(ShaderID::VisualizeVirtualShadowMap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/GUIComposition.hslib", "GUICompositionPS");
            shaderLibrary->LoadShader(ShaderID::GUICompositionPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/DebugVisualization/DebugDrawLines.hslib", "DebugDrawLinesVS");
            shaderLibrary->LoadShader(ShaderID::DebugDrawVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/DebugVisualization/DebugDrawLines.hslib", "DebugDrawLinesPS");
            shaderLibrary->LoadShader(ShaderID::DebugDrawPS, shaderDesc);
        }

        if (false)
        {
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::RayGen, "Shaders/RasterizationRenderer/HardwareRayTracing/RayTracingShadows.hslib", "RayTracingShadowsRayGen");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                //if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                //{
                //    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                //    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                //}
                shaderLibrary->LoadShader(ShaderID::RayTracingShadowsRayGen, shaderDesc);
            }
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Miss, "Shaders/RasterizationRenderer/HardwareRayTracing/RayTracingShadows.hslib", "RayTracingShadowsMiss");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                if (shaderLibrary->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                {
                    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                }
                shaderLibrary->LoadShader(ShaderID::RayTracingShadowsMiss, shaderDesc);
            }

            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/HardwareRayTracing/RayTracingShadows.hslib", "RayTracingShadowsInlineRayTracingCS");
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