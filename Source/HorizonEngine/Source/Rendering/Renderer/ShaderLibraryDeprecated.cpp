#include "ShaderRepository.h"
#include "RasterizationRenderer/RasterizationRenderer.h"

namespace Horizon
{
    static void LoadSkyAtmosphereShaders_Deprecated(ShaderRepository* shaderRepository)
    {
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SkyAtmosphere/SkyAtmosphereTransmittanceLut.hslib", "SkyAtmosphereTransmittanceLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_TRANSMITTANCE_LUT_PASS", 1);
            shaderRepository->LoadShader(ShaderID::SkyAtmosphereTransmittanceLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SkyAtmosphere/SkyAtmosphereMultipleScatteringLut.hslib", "SkyAtmosphereMultipleScatteringLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_MULTIPLE_SCATTERING_LUT_PASS", 1);
            shaderRepository->LoadShader(ShaderID::SkyAtmosphereMultipleScatteringLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SkyAtmosphere/SkyAtmosphereSkyViewLut.hslib", "SkyAtmosphereSkyViewLutCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_SKY_VIEW_LUT_PASS", 1);
            shaderRepository->LoadShader(ShaderID::SkyAtmosphereSkyViewLut, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SkyAtmosphere/SkyAtmosphereAerialPerspectiveVolume.hslib", "SkyAtmosphereAerialPerspectiveVolumeCS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_AERIAL_PERSPECTIVE_VOLUME_PASS", 1);
            shaderRepository->LoadShader(ShaderID::SkyAtmosphereAerialPerspectiveVolume, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/SkyAtmosphere/SkyAtmosphereRayMarching.hslib", "SkyAtmosphereRayMarchingPS");
            shaderDesc.AddDefine("SKY_ATMOSPHERE_RAY_MARCHING_PASS", 1);
            shaderRepository->LoadShader(ShaderID::SkyAtmosphereRayMarching, shaderDesc);
        }
    }

    void LoadAllShaders_Deprecated(ShaderRepository* shaderRepository)
    {
        LoadSkyAtmosphereShaders_Deprecated(shaderRepository);

        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/ImGui.hslib", "ImGuiVS");
            shaderRepository->LoadShader(ShaderID::ImGuiVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/ImGui.hslib", "ImGuiPS");
            shaderRepository->LoadShader(ShaderID::ImGuiPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/DrawFullscreenQuad.hslib", "DrawFullscreenQuadVS");
            shaderRepository->LoadShader(ShaderID::DrawFullscreenQuadVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/LatLongToCubemap.hslib", "LatLongToCubemapCS");
            shaderRepository->LoadShader(ShaderID::LatLongToCubemap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/DownsampleCubemap.hslib", "DownsampleCubemapCS");
            shaderRepository->LoadShader(ShaderID::DownsampleCubemap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/DownsampleTexture2D.hslib", "DownsampleTexture2DCS");
            shaderRepository->LoadShader(ShaderID::DownsampleTexture2DCS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/DownsampleTexture2D.hslib", "DownsampleTexture2DVS");
            shaderRepository->LoadShader(ShaderID::DownsampleTexture2DVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/DownsampleTexture2D.hslib", "DownsampleTexture2DPS");
            shaderRepository->LoadShader(ShaderID::DownsampleTexture2DPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ImageBasedLighting/EnvironmentBRDFIntegration.hslib", "EnvironmentBRDFIntegrationCS");
            shaderRepository->LoadShader(ShaderID::EnvironmentBRDFIntegration, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ImageBasedLighting/EnvironmentMapConvolution.hslib", "EnvironmentMapConvolutionCS");
            shaderRepository->LoadShader(ShaderID::EnvironmentMapConvolution, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ImageBasedLighting/IrradianceEnvironmentMapReference.hslib", "IrradianceEnvironmentMapReferenceCS");
            shaderRepository->LoadShader(ShaderID::IrradianceEnvironmentMapReference, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ImageBasedLighting/IrradianceEnvironmentMapSHOnePass.hslib", "IrradianceEnvironmentMapSHOnePassCS");
            shaderRepository->LoadShader(ShaderID::IrradianceEnvironmentMapSHOnePass, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ImageBasedLighting/IrradianceEnvironmentMapSHSampling.hslib", "IrradianceEnvironmentMapSHSamplingCS");
        //     shaderRepository->LoadShader(ShaderID::IrradianceEnvironmentMapSHSampling, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ImageBasedLighting/IrradianceEnvironmentMapSHIntegration.hslib", "IrradianceEnvironmentMapSHIntegrationCS");
        //     shaderRepository->LoadShader(ShaderID::IrradianceEnvironmentMapSHIntegration, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hslib", "SharedMemoryComplexFFTCS");
            shaderRepository->LoadShader(ShaderID::SharedMemoryComplexFFT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hslib", "SharedMemoryComplexIFFTCS");
            shaderRepository->LoadShader(ShaderID::SharedMemoryComplexIFFT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hslib", "SharedMemoryTwoForOneRealFFTCS");
            shaderRepository->LoadShader(ShaderID::SharedMemoryTwoForOneRealFFT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hslib", "SharedMemoryTwoForOneRealIFFTCS");
            shaderRepository->LoadShader(ShaderID::SharedMemoryTwoForOneRealIFFT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/GPUFFT.hslib", "SharedMemoryComplexFFTConvolutionCS");
            shaderRepository->LoadShader(ShaderID::SharedMemoryComplexFFTConvolution, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VisibilityCulling.hslib", "IndirectArgumentInitializationCS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", MaxTriangleCountPerMeshlet);
            shaderDesc.AddDefine("INDIRECT_ARGUMENT_INITIALIZATION", 1);
            shaderRepository->LoadShader(ShaderID::VisibilityCullingIndirectArgumentInitialization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VisibilityCulling.hslib", "InstanceCullingCS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", MaxTriangleCountPerMeshlet);
            shaderDesc.AddDefine("INSTANCE_CULLING", 1);
            shaderRepository->LoadShader(ShaderID::VirtualGeometryInstanceCulling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VisibilityCulling.hslib", "MeshletCullingCS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", MaxTriangleCountPerMeshlet);
            shaderDesc.AddDefine("MESHLET_CULLING", 1);
            shaderRepository->LoadShader(ShaderID::VirtualGeometryMeshletCulling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VisibilityCulling.hslib", "MeshletGroupCullingCS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", MaxTriangleCountPerMeshlet);
            shaderDesc.AddDefine("MESHLET_GROUP_CULLING", 1);
            shaderRepository->LoadShader(ShaderID::VirtualGeometryMeshletGroupCulling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/VisibilityBuffer.hslib", "VisibilityBufferVS");
            shaderDesc.AddDefine("GPU_SCENE_MAXIMUM_TRIANGLE_COUNT_PER_MESHLET", MaxTriangleCountPerMeshlet);
            shaderRepository->LoadShader(ShaderID::VisibilityBufferVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/VisibilityBuffer.hslib", "VisibilityBufferPS");
            shaderRepository->LoadShader(ShaderID::VisibilityBufferPS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Amplification, "Shaders/RasterizationRenderer/VisibilityBufferMeshShading.hslib", "VisibilityBufferAS");
            //shaderRepository->LoadShader(ShaderID::VisibilityBufferMeshShadingAS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Mesh, "Shaders/RasterizationRenderer/VisibilityBufferMeshShading.hslib", "VisibilityBufferMS");
            //shaderRepository->LoadShader(ShaderID::VisibilityBufferMeshShadingMS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/VisibilityBufferMeshShading.hslib", "VisibilityBufferPS");
            //shaderRepository->LoadShader(ShaderID::VisibilityBufferMeshShadingPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/MotionVectorEstimation.hslib", "MotionVectorEstimationCS");
            shaderRepository->LoadShader(ShaderID::MotionVectorEstimation, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/BuildDepthPyramid.hslib", "BuildDepthPyramidCS");
            shaderDesc.AddDefine("BUILD_DEPTH_PYRAMID_MIN", 1);
            shaderRepository->LoadShader(ShaderID::BuildDepthPyramid, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/GBuffer.hslib", "GBufferCS");
            shaderRepository->LoadShader(ShaderID::GBuffer, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ManyLightRendering/LightGrid/LightGridBufferInitialization.hslib", "LightGridBufferInitializationCS");
            shaderRepository->LoadShader(ShaderID::LightGridBufferInitialization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ManyLightRendering/LightGrid/LightGridLocalLightCulling.hslib", "LightGridLocalLightCullingCS");
            shaderRepository->LoadShader(ShaderID::LightGridLocalLightCulling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ManyLightRendering/LightGrid/LightGridDebugVisualization.hslib", "LightGridDebugVisualizationCS");
            shaderRepository->LoadShader(ShaderID::LightGridDebugVisualization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapClearPageTable.hslib", "VirtualShadowMapClearPageTableCS");
            shaderRepository->LoadShader(ShaderID::VirtualShadowMapClearPageTable, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapPageRequest.hslib", "VirtualShadowMapPageRequestCS");
            shaderRepository->LoadShader(ShaderID::VirtualShadowMapPageRequest, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapPhysicalPageAllocation.hslib", "VirtualShadowMapPhysicalPageAllocationCS");
            shaderRepository->LoadShader(ShaderID::VirtualShadowMapPhysicalPageAllocation, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapClearIndirectArgumentBuffer.hslib", "VirtualShadowMapClearIndirectArgumentBufferCS");
            shaderRepository->LoadShader(ShaderID::VirtualShadowMapClearIndirectArgumentBuffer, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapPhysicalMemoryAllocation.hslib", "VirtualShadowMapPhysicalMemoryAllocationCS");
            shaderRepository->LoadShader(ShaderID::VirtualShadowMapPhysicalMemoryAllocation, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapClearPhysicalMemory.hslib", "VirtualShadowMapClearPhysicalMemoryCS");
            shaderRepository->LoadShader(ShaderID::VirtualShadowMapClearPhysicalMemory, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapInstanceCulling.hslib", "VirtualShadowMapInstanceCullingCS");
            shaderRepository->LoadShader(ShaderID::VirtualShadowMapInstanceCulling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapDepth.hslib", "VirtualShadowMapDepthVS");
            shaderRepository->LoadShader(ShaderID::VirtualShadowMapDepthVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapDepth.hslib", "VirtualShadowMapDepthPS");
            if (shaderRepository->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderRepository->LoadShader(ShaderID::VirtualShadowMapDepthPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VirtualShadowMap/VirtualShadowMapProjection.hslib", "VirtualShadowMapProjectionCS");
            shaderRepository->LoadShader(ShaderID::VirtualShadowMapProjection, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/CascadedShadowMap.hslib", "CascadedShadowMapVS");
            shaderRepository->LoadShader(ShaderID::CascadedShadowMapVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/CascadedShadowMap.hslib", "CascadedShadowMapPS");
            shaderRepository->LoadShader(ShaderID::CascadedShadowMapPS, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/LocalLightShadows.hslib", "LocalLightShadowsVS");
        //     shaderRepository->LoadShader(ShaderID::LocalLightShadowsVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/LocalLightShadows.hslib", "LocalLightShadowsPS");
        //     shaderRepository->LoadShader(ShaderID::LocalLightShadowsPS, shaderDesc);
        // }

        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ShadowMapProjection.hslib", "ShadowMapProjectionForDistantLightCS");
            shaderRepository->LoadShader(ShaderID::ShadowMapProjectionForDistantLight, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceShadows/ScreenSpaceShadowsStochastic.hslib", "ScreenSpaceShadowsStochasticCS");
            shaderRepository->LoadShader(ShaderID::ScreenSpaceShadowsStochastic, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceShadows/ScreenSpaceShadowsBend.hslib", "ScreenSpaceShadowsBendCS");
            shaderRepository->LoadShader(ShaderID::ScreenSpaceShadowsBend, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/ScreenSpaceShadows/ScreenSpaceShadowsComposition.hslib", "ScreenSpaceShadowsCompositionPS");
            shaderRepository->LoadShader(ShaderID::ScreenSpaceShadowsComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceRayTracing/ScreenSpaceIndirectDiffuse.hslib", "ScreenSpaceIndirectDiffuseCS");
            shaderRepository->LoadShader(ShaderID::ScreenSpaceIndirectDiffuse, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceRayTracing/ScreenSpaceReflectionsTileClassification.hslib", "SSRTileClassificationHorizontalCS");
            shaderDesc.AddDefine("SSR_TILE_CLASSIFICATION_HORIZONTAL_PASS", 1);
            shaderRepository->LoadShader(ShaderID::SSRTileClassificationHorizontal, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceRayTracing/ScreenSpaceReflectionsTileClassification.hslib", "SSRTileClassificationVerticalCS");
            shaderDesc.AddDefine("SSR_TILE_CLASSIFICATION_VERTICAL_PASS", 1);
            shaderRepository->LoadShader(ShaderID::SSRTileClassificationVertical, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceRayTracing/ScreenSpaceReflectionsRayAllocation.hslib", "SSRRayAllocationCS");
            shaderRepository->LoadShader(ShaderID::SSRRayAllocation, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceRayTracing/ScreenSpaceReflectionsRayTracing.hslib", "ScreenSpaceReflectionsRayTracingCS");
            shaderDesc.AddDefine("SSR_EARLY_EXIT_RAYS", 1);
            shaderRepository->LoadShader(ShaderID::SSRRayTracingEarlyExit, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceRayTracing/ScreenSpaceReflectionsRayTracing.hslib", "ScreenSpaceReflectionsRayTracingCS");
            shaderDesc.AddDefine("SSR_CHEAP_RAYS", 1);
            shaderRepository->LoadShader(ShaderID::SSRRayTracingCheap, shaderDesc);
        }
        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceReflections.hslib", "ScreenSpaceReflectionsCS");
        //    shaderRepository->LoadShader(ShaderID::SSRRayTracingExpensive, shaderDesc);
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceRayTracing/ScreenSpaceReflectionsColorResolve.hslib", "ScreenSpaceReflectionsColorResolveCS");
            shaderRepository->LoadShader(ShaderID::SSRColorResolve, shaderDesc);
        }
        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceReflectionsTemporalFiltering.hslib", "SSRTemporalFilteringCS");
        //    shaderRepository->LoadShader(ShaderID::SSRTemporalFiltering, shaderDesc);

        //    {ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceReflectionsSpatialFiltering.hslib", "SSRSpatialFilteringCS");
        //    shaderRepository->LoadShader(ShaderID::SSRSpatialFiltering, shaderDesc);
        //}
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceAmbientOcclusion.hslib", "GTAOHorizonSearchIntegralCS");
            shaderDesc.AddDefine("GTAO_HORIZON_SEARCH_INTEGRAL", 1);
            shaderRepository->LoadShader(ShaderID::GTAOHorizonSearchIntegral, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceAmbientOcclusion.hslib", "GTAOSpatialFilteringCS");
            shaderDesc.AddDefine("GTAO_SPATIAL_FILTERING", 1);
            shaderRepository->LoadShader(ShaderID::GTAOSpatialFiltering, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceAmbientOcclusion.hslib", "GTAOTemporalFilteringCS");
            shaderDesc.AddDefine("GTAO_TEMPORAL_FILTERING", 1);
            shaderRepository->LoadShader(ShaderID::GTAOTemporalFiltering, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceAmbientOcclusion.hslib", "GTAOUpsamplingCS");
            shaderDesc.AddDefine("GTAO_UPSAMPLING", 1);
            shaderRepository->LoadShader(ShaderID::GTAOUpsampling, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/IndirectDiffuseComposition.hslib", "IndirectDiffuseCompositionPS");
            shaderRepository->LoadShader(ShaderID::IndirectDiffuseComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/IndirectSpecularComposition.hslib", "IndirectSpecularCompositionPS");
            shaderRepository->LoadShader(ShaderID::IndirectSpecularComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/DirectLighting.hslib", "DirectLightingPS");
            if (shaderRepository->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderRepository->LoadShader(ShaderID::DirectLighting, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/SkyBox.hslib", "SkyBoxVS");
        //     shaderRepository->LoadShader(ShaderID::SkyBoxVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/SkyBox.hslib", "SkyBoxPS");
        //     shaderRepository->LoadShader(ShaderID::SkyBoxPS, shaderDesc);
        // }
        {
           ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringInitialization.hslib", "SubsurfaceScatteringInitializationCS");
           shaderRepository->LoadShader(ShaderID::SubsurfaceScatteringInitialization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringTileClassification.hslib", "SubsurfaceScatteringTileClassificationCS");
            shaderRepository->LoadShader(ShaderID::SubsurfaceScatteringTileClassification, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringBuildIndirectArguments.hslib", "SubsurfaceScatteringBuildIndirectArgumentsCS");
            shaderRepository->LoadShader(ShaderID::SubsurfaceScatteringBuildIndirectArguments, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringConvolution.hslib", "SubsurfaceScatteringConvolutionCS");
            shaderRepository->LoadShader(ShaderID::SubsurfaceScatteringConvolution, shaderDesc);
        }
        //{
        //    ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringComputeVariance.hslib", "SubsurfaceScatteringComputeVarianceCS");
        //    shaderRepository->LoadShader(ShaderID::SubsurfaceScatteringComputeVariance, shaderDesc);
        //}
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringLightingComposition.hslib", "SubsurfaceScatteringLightingCompositionVS");
            shaderRepository->LoadShader(ShaderID::SubsurfaceScatteringLightingCompositionVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringLightingComposition.hslib", "SubsurfaceScatteringLightingCompositionPS");
            shaderRepository->LoadShader(ShaderID::SubsurfaceScatteringLightingCompositionPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringCopyResults.hslib", "SubsurfaceScatteringCopyResultsVS");
            shaderRepository->LoadShader(ShaderID::SubsurfaceScatteringCopyResultsVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/SubsurfaceScattering/SubsurfaceScatteringCopyResults.hslib", "SubsurfaceScatteringCopyResultsPS");
            shaderRepository->LoadShader(ShaderID::SubsurfaceScatteringCopyResultsPS, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIFreeSurfels.hslib", "SurfelGIFreeSurfelsCS");
        //     shaderRepository->LoadShader(ShaderID::SurfelGIFreeSurfels, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIGapFilling.hslib", "SurfelGIGapFillingCS");
        //     shaderRepository->LoadShader(ShaderID::SurfelGIGapFilling, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIIndirectArguments.hslib", "SurfelGIIndirectArgumentsCS");
        //     shaderRepository->LoadShader(ShaderID::SurfelGIIndirectArguments, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIGridReset.hslib", "SurfelGIGridResetCS");
        //     shaderRepository->LoadShader(ShaderID::SurfelGIGridReset, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIComputeCellCapacity.hslib", "SurfelGIComputeCellCapacityCS");
        //     shaderRepository->LoadShader(ShaderID::SurfelGIComputeCellCapacity, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIComputeCellOffset.hslib", "SurfelGIComputeCellOffsetCS");
        //     shaderRepository->LoadShader(ShaderID::SurfelGIComputeCellOffset, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/SurfelGI/SurfelGIVisualization.hslib", "SurfelGIVisualizationCS");
        //     shaderRepository->LoadShader(ShaderID::SurfelGIVisualization, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceLightShafts/ScreenSpaceLightShaftsDownsample.hslib", "ScreenSpaceLightShaftsDownsampleCS");
            shaderRepository->LoadShader(ShaderID::ScreenSpaceLightShaftsDownsample, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceLightShafts/ScreenSpaceLightShaftsTemporalFiltering.hslib", "ScreenSpaceLightShaftsTemporalFilteringCS");
            shaderRepository->LoadShader(ShaderID::ScreenSpaceLightShaftsTemporalFiltering, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/ScreenSpaceLightShafts/ScreenSpaceLightShaftsRadialBlur.hslib", "ScreenSpaceLightShaftsRadialBlurCS");
            shaderRepository->LoadShader(ShaderID::ScreenSpaceLightShaftsRadialBlur, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/ScreenSpaceLightShafts/ScreenSpaceLightShaftsComposition.hslib", "ScreenSpaceLightShaftsCompositionPS");
            shaderRepository->LoadShader(ShaderID::ScreenSpaceLightShaftsComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VolumetricFog/VolumetricFogVoxelization.hslib", "VolumetricFogVoxelizationCS");
            shaderRepository->LoadShader(ShaderID::VolumetricFogVoxelization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VolumetricFog/VolumetricFogLightScattering.hslib", "VolumetricFogLightScatteringCS");
            shaderRepository->LoadShader(ShaderID::VolumetricFogLightScattering, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/VolumetricFog/VolumetricFogFinalIntegration.hslib", "VolumetricFogFinalIntegrationCS");
            shaderRepository->LoadShader(ShaderID::VolumetricFogFinalIntegration, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/VolumetricFog/VolumetricFogComposition.hslib", "VolumetricFogCompositionPS");
            shaderRepository->LoadShader(ShaderID::VolumetricFogComposition, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/VolumetricFog/LocalFogVolume.hslib", "LocalFogVolumeVS");
            shaderRepository->LoadShader(ShaderID::LocalFogVolumeVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/VolumetricFog/LocalFogVolume.hslib", "LocalFogVolumePS");
            shaderRepository->LoadShader(ShaderID::LocalFogVolumePS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/MotionBlur/MotionBlurTileClassification.hslib", "MotionBlurTileClassificationCS");
            shaderRepository->LoadShader(ShaderID::MotionBlurTileClassificationCS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/PostProcessing/MotionBlur/MotionBlurVelocityDilation.hslib", "MotionBlurVelocityDilationScatterVS");
            shaderRepository->LoadShader(ShaderID::MotionBlurVelocityDilationScatterVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/PostProcessing/MotionBlur/MotionBlurVelocityDilation.hslib", "MotionBlurVelocityDilationScatterPS");
            shaderRepository->LoadShader(ShaderID::MotionBlurVelocityDilationScatterPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/MotionBlur/MotionBlurReconstructionFilter.hslib", "MotionBlurReconstructionFilterCS");
            shaderRepository->LoadShader(ShaderID::MotionBlurReconstructionFilterCS, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/TemporalSuperSampling/TemporalSuperSampling.hslib", "TemporalSuperSamplingCS");
            //shaderRepository->LoadShader(ShaderID::TemporalSuperSampling, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/DepthOfFieldSetup.hslib", "DepthOfFieldSetupCS");
            //shaderRepository->LoadShader(ShaderID::DepthOfFieldSetup, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/DepthOfFieldGather.hslib", "DepthOfFieldGatherCS");
            //shaderRepository->LoadShader(ShaderID::DepthOfFieldGather, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/DepthOfFieldPostfilter.hslib", "DepthOfFieldPostfilterCS");
            //shaderRepository->LoadShader(ShaderID::DepthOfFieldPostfilter, shaderDesc);
        }
        {
            //ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/DepthOfFieldRecombine.hslib", "DepthOfFieldRecombineCS");
            //shaderRepository->LoadShader(ShaderID::DepthOfFieldRecombine, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/AutoExposureBuildHistogram.hslib", "AutoExposureBuildHistogramCS");
            if (shaderRepository->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderRepository->LoadShader(ShaderID::AutoExposureBuildHistogram, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/AutoExposureComputeExposure.hslib", "AutoExposureComputeExposureCS");
            shaderRepository->LoadShader(ShaderID::AutoExposureComputeExposure, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/CopyExposure.hslib", "CopyExposureCS");
            shaderRepository->LoadShader(ShaderID::CopyExposure, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/BuildColorPyramid.hslib", "BuildColorPyramidCS");
            shaderRepository->LoadShader(ShaderID::BuildColorPyramid, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/GaussianBloomDownsample.hslib", "GaussianBloomDownsampleCS");
            shaderRepository->LoadShader(ShaderID::GaussianBloomDownsample, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/GaussianBloomUpsample.hslib", "GaussianBloomUpsampleCS");
            shaderRepository->LoadShader(ShaderID::GaussianBloomUpsample, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/ConvolutionBloomResizeKernel.hslib", "ConvolutionBloomResizeKernelCS");
            shaderRepository->LoadShader(ShaderID::ConvolutionBloomResizeKernel, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/PostProcessing/LensFlareGhost.hslib", "LensFlareGhostPS");
            shaderRepository->LoadShader(ShaderID::LensFlareGhost, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LensFlareTileCulling.hslib", "LensFlareTileCullingCS");
        //     shaderRepository->LoadShader(ShaderID::LensFlareTileCulling, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/PostProcessing/LensFlareGlare.hslib", "LensFlareGlareVS");
        //     shaderRepository->LoadShader(ShaderID::LensFlareGlareVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/PostProcessing/LensFlareGlare.hslib", "LensFlareGlarePS");
        //     shaderRepository->LoadShader(ShaderID::LensFlareGlarePS, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/PostProcessing/LensFlareCombine.hslib", "LensFlareCombinePS");
            shaderRepository->LoadShader(ShaderID::LensFlareCombine, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingBuildGrid.hslib", "BilateralGridLocalToneMappingBuildGridCS");
            shaderRepository->LoadShader(ShaderID::BilateralGridLocalToneMappingBuildGrid, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingComputeLogLuminance.hslib", "BilateralGridLocalToneMappingComputeLogLuminanceCS");
            shaderRepository->LoadShader(ShaderID::BilateralGridLocalToneMappingComputeLogLuminance, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingGaussianDistribution.hslib", "BilateralGridLocalToneMappingGaussianDistributionCS");
            shaderRepository->LoadShader(ShaderID::BilateralGridLocalToneMappingGaussianDistribution, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingGaussianFilter.hslib", "BilateralGridLocalToneMappingGaussianFilterCS");
            shaderRepository->LoadShader(ShaderID::BilateralGridLocalToneMappingGaussianFilter, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/BilateralGridLocalToneMappingUpsampling.hslib", "BilateralGridLocalToneMappingUpsamplingCS");
            shaderRepository->LoadShader(ShaderID::BilateralGridLocalToneMappingUpsampling, shaderDesc);
        }
         {
             ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/ExposureFusionComputeLuminanceAndWeight.hslib", "ExposureFusionComputeLuminanceAndWeightCS");
             shaderRepository->LoadShader(ShaderID::ExposureFusionComputeLuminanceAndWeight, shaderDesc);
         }
         {
             ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/LocalToneMappingBlendExposures.hslib", "LocalToneMappingBlendExposuresCS");
             shaderRepository->LoadShader(ShaderID::LocalToneMappingBlendExposures, shaderDesc);
         }
         {
             ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/LocalToneMappingBlendLaplacian.hslib", "LocalToneMappingBlendLaplacianCS");
             shaderRepository->LoadShader(ShaderID::LocalToneMappingBlendLaplacian, shaderDesc);
         }
         {
             ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/LocalToneMapping/ExposureFusionGuidedUpsampling.hslib", "ExposureFusionGuidedUpsamplingCS");
             shaderRepository->LoadShader(ShaderID::ExposureFusionGuidedUpsampling, shaderDesc);
         }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/ColorTransformLUT.hslib", "ColorTransformLUTCS");
            if (shaderRepository->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderRepository->LoadShader(ShaderID::ColorTransformLUT, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/FinalComposition.hslib", "FinalCompositionCS");
            shaderRepository->LoadShader(ShaderID::FinalComposition, shaderDesc);
        }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/PostProcessing/SelectionOutlineMask.hslib", "SelectionOutlineMaskVS");
        //     shaderRepository->LoadShader(ShaderID::SelectionOutlineMaskVS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/PostProcessing/SelectionOutlineMask.hslib", "SelectionOutlineMaskPS");
        //     shaderRepository->LoadShader(ShaderID::SelectionOutlineMaskPS, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/SelectionOutlineSetup.hslib", "SelectionOutlineSetupCS");
        //     shaderRepository->LoadShader(ShaderID::SelectionOutlineSetup, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/SelectionOutlineJumpFlood.hslib", "SelectionOutlineJumpFloodCS");
        //     shaderRepository->LoadShader(ShaderID::SelectionOutlineJumpFlood, shaderDesc);
        // }
        // {
        //     ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/PostProcessing/SelectionOutlineComposite.hslib", "SelectionOutlineCompositeCS");
        //     shaderRepository->LoadShader(ShaderID::SelectionOutlineComposite, shaderDesc);
        // }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeDepth.hslib", "VisualizeDepthCS");
            shaderRepository->LoadShader(ShaderID::VisualizeDepth, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VirtualGeometryDebugVisualization.hslib", "VirtualGeometryDebugVisualizationCS");
            if (shaderRepository->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
            {
                shaderDesc.shaderCompilerOptions.skipOptimization = false;
                shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
            }
            shaderRepository->LoadShader(ShaderID::VirtualGeometryDebugVisualization, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeWorldSpaceNormal.hslib", "VisualizeWorldSpaceNormalCS");
            shaderRepository->LoadShader(ShaderID::VisualizeWorldSpaceNormal, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeMotionVectors.hslib", "VisualizeMotionVectorsCS");
            shaderRepository->LoadShader(ShaderID::VisualizeMotionVectors, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeAmbientOcclusion.hslib", "VisualizeAmbientOcclusionCS");
            shaderRepository->LoadShader(ShaderID::VisualizeAmbientOcclusion, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeScreenSpaceShadowMask.hslib", "VisualizeScreenSpaceShadowMaskCS");
            shaderRepository->LoadShader(ShaderID::VisualizeScreenSpaceShadowMask, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeCascadedShadowMap.hslib", "VisualizeCascadedShadowMapCS");
            shaderRepository->LoadShader(ShaderID::VisualizeCascadedShadowMap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/DebugVisualization/VisualizeVirtualShadowMap.hslib", "VisualizeVirtualShadowMapCS");
            shaderRepository->LoadShader(ShaderID::VisualizeVirtualShadowMap, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/GUIComposition.hslib", "GUICompositionPS");
            shaderRepository->LoadShader(ShaderID::GUICompositionPS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Vertex, "Shaders/RasterizationRenderer/DebugVisualization/DebugDrawLines.hslib", "DebugDrawLinesVS");
            shaderRepository->LoadShader(ShaderID::DebugDrawVS, shaderDesc);
        }
        {
            ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Pixel, "Shaders/RasterizationRenderer/DebugVisualization/DebugDrawLines.hslib", "DebugDrawLinesPS");
            shaderRepository->LoadShader(ShaderID::DebugDrawPS, shaderDesc);
        }

        if (true)
        {
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::RayGen, "Shaders/RasterizationRenderer/HardwareRayTracing/RayTracingShadows.hslib", "RayTracingShadowsRayGen");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                //if (shaderRepository->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                //{
                //    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                //    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                //}
                shaderRepository->LoadShader(ShaderID::RayTracingShadowsRayGen, shaderDesc);
            }
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Miss, "Shaders/RasterizationRenderer/HardwareRayTracing/RayTracingShadows.hslib", "RayTracingShadowsMiss");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                if (shaderRepository->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                {
                    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                }
                shaderRepository->LoadShader(ShaderID::RayTracingShadowsMiss, shaderDesc);
            }
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Compute, "Shaders/RasterizationRenderer/HardwareRayTracing/RayTracingShadows.hslib", "RayTracingShadowsInlineRayTracingCS");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                //shaderDesc.shaderCompilerOptions.inlineRayTracing = true;
                //if (shaderRepository->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                {
                    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                }
                shaderRepository->LoadShader(ShaderID::RayTracingShadowsInlineRayTracing, shaderDesc);
            }
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::RayGen, "Shaders/PathTracingRenderer/PathTracing.hslib", "PathTracingRayGen");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                if (shaderRepository->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                {
                    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                }
                shaderRepository->LoadShader(ShaderID::PathTracingRayGen, shaderDesc);
            }
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Miss, "Shaders/PathTracingRenderer/PathTracing.hslib", "PathTracingDefaultMiss");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                if (shaderRepository->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                {
                    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                }
                shaderRepository->LoadShader(ShaderID::PathTracingDefaultMiss, shaderDesc);
            }
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::Miss, "Shaders/PathTracingRenderer/PathTracing.hslib", "PathTracingShadowRayMiss");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                if (shaderRepository->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                {
                    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                }
                shaderRepository->LoadShader(ShaderID::PathTracingShadowRayMiss, shaderDesc);
            }
            {
                ShaderDesc shaderDesc = ShaderDesc::Create(ShaderStage::ClosestHit, "Shaders/PathTracingRenderer/PathTracing.hslib", "PathTracingDefaultOpaqueClosestHit");
                shaderDesc.AddDefine("RAY_TRACING_ENABLED", 1);
                if (shaderRepository->renderBackend->GetType() == RenderBackendType::Vulkan) // Avoid vulkan validation errors.
                {
                    shaderDesc.shaderCompilerOptions.skipOptimization = false;
                    shaderDesc.shaderCompilerOptions.generateDebugInfo = false;
                }
                shaderRepository->LoadShader(ShaderID::PathTracingDefaultOpaqueClosestHit, shaderDesc);
            }
        }
    }
}