#pragma once

#include "RendererCommon.h"

// Currently, shader permutations are not supported.
// https://therealmjp.github.io/posts/shader-permutations-part1/
// https://therealmjp.github.io/posts/shader-permutations-part2/

namespace Horizon
{
    enum class ShaderID : uint32
    {
        ImGuiVS,
        ImGuiPS,
        FullScreenQuadVS,
        LatLongToCubemap,
        DownsampleCubemap,
        DownsampleTexture2DCS,
        DownsampleTexture2DVS,
        DownsampleTexture2DPS,
        EnvironmentBRDFIntegration,
        EnvironmentMapConvolution,
        IrradianceEnvironmentMapReference,
        IrradianceEnvironmentMapSHOnePass,
        IrradianceEnvironmentMapSHSampling,
        IrradianceEnvironmentMapSHIntegration,
        SharedMemoryComplexFFT,
        SharedMemoryComplexIFFT,
        SharedMemoryTwoForOneRealFFT,
        SharedMemoryTwoForOneRealIFFT,
        SharedMemoryComplexFFTConvolution,
        // Begin: Real Time Renderer
        VisibilityBufferVS,
        VisibilityBufferPS,
        VisibilityBufferMeshShadingAS,
        VisibilityBufferMeshShadingMS,
        VisibilityBufferMeshShadingPS,
        GBuffer,
        LightGridBufferInitialization,
        LightGridLocalLightCulling,
        LightGridDebugVisualization,
        BuildDepthPyramid,
        VirtualShadowMapPageRequest,
        VirtualShadowMapClearIndirectArgumentBuffer,
        VirtualShadowMapPhysicalMemoryAllocation,
        VirtualShadowMapClearPhysicalMemory,
        VirtualShadowMapDepthVS,
        VirtualShadowMapDepthPS,
        VirtualShadowMapProjection,
        CascadedShadowMapVS,
        CascadedShadowMapPS,
        LocalLightShadowsVS,
        LocalLightShadowsPS,
        RayTracedShadowMap,
        ShadowMapProjectionForDistantLight,
        MotionVectors,
        SurfelGIFreeSurfels,
        SurfelGIGapFilling,
        SurfelGIIndirectArguments,
        SurfelGIGridReset,
        SurfelGIComputeCellCapacity,
        SurfelGIComputeCellOffset,
        SurfelGIVisualization,
        GTAOHorizonSearchAndIntegral,
        GTAOSpatialFiltering,
        GTAOTemporalFiltering,
        IndirectDiffuseComposition,
        IndirectSpecularComposition,
        ScreenSpaceIndirectDiffuse,
        SSRTileClassificationHorizontal,
        SSRTileClassificationVertical,
        SSRRayAllocation,
        SSRRayTracingEarlyExit,
        SSRRayTracingCheap,
        SSRRayTracingExpensive,
        SSRColorResolve,
        SSRTemporalFiltering,
        SSRSpatialFiltering,
        DirectLighting,
        SkyBoxVS,
        SkyBoxPS,
        SubsurfaceScatteringInitialize,
        SubsurfaceScatteringClassifyTiles,
        SubsurfaceScatteringBuildIndirectArguments,
        SubsurfaceScatteringSampleDiffusionProfile,
        SubsurfaceScatteringComputeVariance,
        SubsurfaceScatteringRecombineVS,
        SubsurfaceScatteringRecombinePS,
        SubsurfaceScatteringCopyResultsVS,
        SubsurfaceScatteringCopyResultsPS,
        SkyAtmosphereTransmittanceLut,
        SkyAtmosphereMultipleScatteringLut,
        SkyAtmosphereSkyViewLut,
        SkyAtmosphereAerialPerspectiveVolume,
        SkyAtmosphereRayMarching,
        LightShaftsDownsample,
        LightShaftsRadialBlur,
        LightShaftsApply,
        VolumetricFogVoxelization,
        VolumetricFogLightScattering,
        VolumetricFogFinalIntegration,
        VolumetricFogComposition,
        LocalFogVolumeVS,
        LocalFogVolumePS,
        TemporalSuperSampling,
        MotionBlurSetupCS,
        MotionBlurVelocityDilationScatterVS,
        MotionBlurVelocityDilationScatterPS,
        MotionBlurReconstructionFilterCS,
        DepthOfFieldSetup,
        DepthOfFieldGather,
        DepthOfFieldPostfilter,
        DepthOfFieldRecombine,
        AutoExposureBuildHistogram,
        AutoExposureComputeExposure,
        CopyExposure,
        BuildColorPyramid,
        GaussianBloomDownsample,
        GaussianBloomUpsample,
        ConvolutionBloomResizeKernel,
        LensFlareGhost,
        LensFlareTileCulling,
        LensFlareGlareVS,
        LensFlareGlarePS,
        LensFlareCombine,
        LocalExposureComputeLuminance,
        LocalExposureComputeWeights,
        LocalExposureBlendExposures,
        LocalExposureBlendLaplacian,
        LocalExposureGuidedUpsampling,
        ColorLUT,
        ToneMapping,
        SelectionOutlineMaskVS,
        SelectionOutlineMaskPS,
        SelectionOutlineSetup,
        SelectionOutlineJumpFlood,
        SelectionOutlineComposite,
        VisualizeDepth,
        VisualizePrimitiveID,
        VisualizeMaterialID,
        VisualizeWorldSpaceNormal,
        VisualizeMotionVectors,
        VisualizeAmbientOcclusion,
        VisualizeScreenSpaceShadowMask,
        GUICompositionPS,
        DebugDrawVS,
        DebugDrawPS,
        // End: Real Time Renderer
        Count,
    };

    struct ShaderDesc
    {
        ShaderStage stage;
        const char* filename;
        const char* entryFunctionName;
        std::vector<ShaderMacroDefine> defines;

        static ShaderDesc Create(ShaderStage stage, const char* filename, const char* entryFunctionName)
        {
            ShaderDesc desc;
            desc.stage = stage;
            desc.filename = filename;
            desc.entryFunctionName = entryFunctionName;
            return desc;
        }

        void AddDefine(const char* name, uint32 value)
        {
            ShaderMacroDefine& define = defines.emplace_back();
            define.name = name;
            define.value = std::format("{}", value);
        }
    };

    struct Shader
    {
        ShaderID id;
        ShaderDesc desc;
        RenderBackendShaderHandle handle;
        std::vector<std::filesystem::path> relatedFiles;
        std::vector<std::chrono::time_point<std::chrono::file_clock>> lastModifiedTime;
        bool compiled;
    };

    class ShaderLibrary
    {
    public:
        ShaderLibrary(RenderBackend* renderBackend, const std::string& rootDirectory);
        virtual ~ShaderLibrary();
        bool HotReload();
        bool LoadShader(ShaderID id, ShaderDesc& desc);
        RenderBackendShaderHandle GetShader(ShaderID id) const;
    private:
        std::string rootDirectory;
        RenderBackend* renderBackend;
        ShadingLanguage shadingLanguage;
        ShaderCompilerOptions shaderCompilerOptions;
        std::vector<Shader> loadedShaders;

        /** Experimental */
        bool hotReloadEnabled;
    };

    extern void LoadAllShaders_Deprecated(ShaderLibrary* shaderLibrary);
}