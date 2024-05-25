#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    enum class ShaderID : uint32
    {
        PreIntegratedBRDF,
        LatLongToCubemap,
        DownsampleCubemap,
        DownsampleTexture2D,
        DownsampleTexture2D_PS,
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
        std::string filename;
        std::string entryFunctionName;
        std::vector<ShaderMacroDefine> defines;

        static ShaderDesc CreateCompute(const std::string& filename, const std::string& csMain)
        {
            ShaderDesc desc;
            desc.type = ShaderDesc::Type::Compute;
            desc.filename = filename;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Compute] = csMain;
            return desc;
        }

        static ShaderDesc CreateGraphics(const std::string& filename, const std::string& vsMain, const std::string& psMain)
        {
            ShaderDesc desc;
            desc.type = ShaderDesc::Type::Graphics;
            desc.filename = filename;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Vertex] = vsMain;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel] = psMain;
            return desc;
        }

        static ShaderDesc CreateMesh(const std::string& filename, const std::string& tsMain, const std::string& msMain, const std::string& psMain)
        {
            ShaderDesc desc;
            desc.type = ShaderDesc::Type::Mesh;
            desc.filename = filename;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Task] = tsMain;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Mesh] = msMain;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel] = psMain;
            return desc;
        }

        static ShaderDesc CreateMesh(const std::string& filename, const std::string& msMain, const std::string& psMain)
        {
            ShaderDesc desc;
            desc.type = ShaderDesc::Type::Mesh;
            desc.filename = filename;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Mesh] = msMain;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel] = psMain;
            return desc;
        }

        static ShaderDesc CreateRayTracing()
        {
            // TODO
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

    // Currently, shader permutations are not supported.
    // https://therealmjp.github.io/posts/shader-permutations-part1/
    // https://therealmjp.github.io/posts/shader-permutations-part2/

    class ShaderLibrary
    {
    public:
        ShaderLibrary(RenderBackend* renderBackend, const char* rootDirectory);
        virtual ~ShaderLibrary();
        bool HotReload();
        bool LoadShader(ShaderID id, ShaderDesc& desc);
        RenderBackendShaderHandle GetShader(ShaderID id);
    private:
        const char* rootDirectory;
        RenderBackend* renderBackend;
        ShadingLanguage shadingLanguage;
        ShaderCompilerOptions shaderCompilerOptions;
        std::vector<Shader> loadedShaders;
        bool hotReloadEnabled;
    };

    extern void CompileShaders_Deprecated(ShaderLibrary* shaderLibrary);
}