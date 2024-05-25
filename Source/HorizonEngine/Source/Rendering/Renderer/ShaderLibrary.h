#pragma once

#include "RendererCommon.h"

// Currently, shader permutations are not supported.
// https://therealmjp.github.io/posts/shader-permutations-part1/
// https://therealmjp.github.io/posts/shader-permutations-part2/

namespace Horizon
{
    enum class ShaderID : uint32
    {
        FullScreenQuadVS,
        PreIntegratedBRDF,
        LatLongToCubemap,
        DownsampleCubemap,
        DownsampleTexture2DCS,
        DownsampleTexture2DVS,
        DownsampleTexture2DPS,
        ComputeEnvironmentIrradiance,
        ComputeEnvironmentIrradianceSH,
        FilterEnvironmentMap,
        SharedMemoryComplexFFT,
        SharedMemoryComplexIFFT,
        SharedMemoryTwoForOneRealFFT,
        SharedMemoryTwoForOneRealIFFT,
        SharedMemoryComplexFFTConvolution,
        UIColorAndAlpha,
        // Begin: Real Time Renderer
        VBuffer,
        VBufferMeshlet,
        GBuffer,
        BuildHZB,
        CascadedShadowMap,
        RayTracedShadowMap,
        CubeShadowMap,
        LocalLightShadows,
        ScreenSpaceShadowsDirectionalLight,
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
        IndirectLightingDiffuse,
        IndirectLightingSpecular,
        SSRTileClassificationHorizontal,
        SSRTileClassificationVertical,
        SSRRayAllocation,
        SSRDispatchEarlyExitRays,
        SSRDispatchCheapRays,
        SSRDispatchExpensiveRays,
        SSRResolve,
        SSRTemporalFiltering,
        SSRSpatialFiltering,
        DirectLighting,
        SkyBox,
        SubsurfaceScatteringSetup,
        SubsurfaceScatteringClassifyTiles,
        SubsurfaceScatteringBuildIndirectArguments,
        SubsurfaceScatteringSampleDiffusionProfile,
        SubsurfaceScatteringComputeVariance,
        SubsurfaceScatteringRecombine,
        SubsurfaceScatteringCopyResults,
        SkyAtmosphereTransmittanceLut,
        SkyAtmosphereMultipleScatteringLut,
        SkyAtmosphereSkyViewLut,
        SkyAtmosphereAerialPerspectiveVolume,
        SkyAtmosphereRayMarching,
        LightShaftsDownsample,
        LightShaftsRadialBlur,
        LightShaftsApply,
        TemporalSuperSampling,
        DepthOfFieldSetup,
        DepthOfFieldGather,
        DepthOfFieldPostfilter,
        DepthOfFieldRecombine,
        AutoExposureBuildHistogram,
        AutoExposureComputeExposure,
        Downsample,
        GaussianBloomDownsample,
        GaussianBloomUpsample,
        ConvolutionBloomResizeKernel,
        LensFlaresGhost,
        LensFlaresTileCulling,
        LensFlaresGlare,
        LensFlaresCombine,
        LocalExposureComputeLuminance,
        LocalExposureComputeWeights,
        LocalExposureBlendExposures,
        LocalExposureBlendLaplacian,
        LocalExposureGuidedUpsampling,
        ColorLUT,
        ToneMapping,
        EditorSelectionOutlineMaskGen,
        EditorSelectionOutlineSetup,
        EditorSelectionOutlineJumpFlood,
        EditorSelectionOutlineComposite,
        VisualizePrimitiveID,
        VisualizeMaterialID,
        VisualizeWorldSpaceNormal,
        VisualizeMotionVectors,
        VisualizeAmbientOcclusion,
        VisualizeScreenSpaceShadowMask,
        DebugDraw,
        GUIComposition,
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