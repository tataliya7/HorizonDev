#pragma once

#include "Rendering/Renderer/RealTimeRenderer/RealTimeRendererCommon.h"
#include "Rendering/Renderer/RealTimeRenderer/PostProcessing/PostProcessing.h"
#include "Rendering/Renderer/RealTimeRenderer/SkyAtmosphere.h"

namespace HE
{
    static inline uint8 GetStencilBitMask(uint32 bit)
    {
        return uint8(uint8(0x01) << bit);
    }

    /*
     * Stencil layout:
     * Bit | Usage
     * [0] | Unused
     * [1] | Unused
     * [2] | Unused
     * [3] | Unused
     * [4] | Unused
     * [5] | Unused
     * [6] | Unused
     * [7] | Unused
     */
    static const uint8 StencilBit0Mask = GetStencilBitMask(0);
    static const uint8 StencilBit1Mask = GetStencilBitMask(1);
    static const uint8 StencilBit2Mask = GetStencilBitMask(2);
    static const uint8 StencilBit3Mask = GetStencilBitMask(3);
    static const uint8 StencilBit4Mask = GetStencilBitMask(4);
    static const uint8 StencilBit5Mask = GetStencilBitMask(5);
    static const uint8 StencilBit6Mask = GetStencilBitMask(6);
    static const uint8 StencilBit7Mask = GetStencilBitMask(7);

    // TODO
    static const uint32 SurfelGIIndirectDispatchGroupThreadCount = 64;
    static const uint32 SurfelGIMaxSurfelCount = 100000;
    static const uint32 SurfelGIMaxSurfelCountPerCell = 10;
    static const uint32 SurfelGIUniformGridCellSize = 2;
    static const Vector3i SurfelGIUniformGridSize = Vector3i(128, 128, 64); // The size of each dimension must be a multiple of 2!
    static const uint32 SurfelGIUniformGridCellCount = SurfelGIUniformGridSize.x * SurfelGIUniformGridSize.y * SurfelGIUniformGridSize.z;
    static const float SurfelGICoverageThreshold = 0.5f;
    static const uint32 SurfelGIScreenTileSize = 16;

    static const uint32 SurfelGIInfoBufferOffset_AliveSurfelCount = 0;
    static const uint32 SurfelGIInfoBufferOffset_DeadSurfelCount = SurfelGIInfoBufferOffset_AliveSurfelCount + 4;
    static const uint32 SurfelGIInfoBufferSize = SurfelGIInfoBufferOffset_DeadSurfelCount + 4;

    // Surfel hot data
    struct SurfelHotData
    {
        Vector3 position;
        Vector3 normal;
        float radius;
    };

    // Surfel cold data
    struct SurfelColdData
    {
        Vector3 position;
        Vector3 normal;
        Vector2i primitiveID;
    };

    struct CellHeader
    {
        uint32 capacity;
        uint32 offset;
    };

    enum class RealTimeRendererShaderPiplineID
    {
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
        SurfelGIIndirectAruguments,
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
        LocalExposureComputeLuminances,
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
        Count
    };

    struct RealTimeRendererSceneViewShaderParameters
    {
        uint32 frameIndex;

        float blueNoisePhase;
        float deltaTime;
        float gamma;
        float exposure;
        float mipLodBias;

        float sceneColorPreExposure;
        float sceneColorOneOverPreExposure;
        float historySceneColorPreExposureCorrection;

        Vector3 skyAtmosphereLightOuterSpaceIlluminance;
        Vector4 skyAtmospherePlanetCenterAndViewHeight;
        float skyAtmosphereBottomRadiusInKilometers;
        float skyAtmosphereTopRadiusInKilometers;
        Matrix3x3 skyAtmosphereSkyViewLUTReferential;
        Vector3 skyAtmosphereSkyLuminanceFactor;

        Vector3 atmosphereLightDirection;
        Vector3 atmosphereLightDiscLuminance;
        float atmosphereLightDiscCosHalfApexAngle;

        Vector3 indirectLightingFactor;

        Vector3 cameraPosition;
        Vector3 prevCameraPosition;

        Vector3 cameraUp;
        Vector3 cameraRight;
        Vector3 cameraForward;

        Vector2 cameraJitterOffset;
        Vector2 prevCameraJitterOffset;

        float cameraNearPlane;
        float cameraFarPlane;
        float cameraHalfFovRad;
        float cameraAspectRatio;

        Vector4 frustumPlanes[6];

        Matrix4x4 viewMatrix;
        Matrix4x4 invViewMatrix;

        Matrix4x4 projectionMatrix;
        Matrix4x4 inverseProjectionMatrix;
        Matrix4x4 viewProjectionMatrix;
        Matrix4x4 invViewProjectionMatrix;

        Matrix4x4 prevProjectionMatrix;
        Matrix4x4 prevViewProjectionMatrix;
        Matrix4x4 prevInvViewProjectionMatrix;

        Matrix4x4 nonJitteredProjectionMatrix;
        Matrix4x4 nonJitteredInvProjectionMatrix;
        Matrix4x4 nonJitteredViewProjectionMatrix;
        Matrix4x4 nonJitteredInvViewProjectionMatrix;

        Matrix4x4 nonJitteredPrevProjectionMatrix;
        Matrix4x4 nonJitteredPrevViewProjectionMatrix;
        Matrix4x4 nonJitteredPrevInvViewProjectionMatrix;

        uint32 renderResolutionX;
        uint32 renderResolutionY;
        uint32 targetResolutionX;
        uint32 targetResolutionY;

        Vector4 renderResolutionAndInvRenderResolution;
        Vector4 targetResolutionAndInvTargetResolution;

        float fixedExposureValue;
        float autoExposureExposureCompensation;
        float autoExposureMinExposureValue;
        float autoExposureMaxExposureValue;
        float autoExposureHistogramLowPercent;
        float autoExposureHistogramHighPercent;
        float autoExposureHistogramMinEV100;
        float autoExposureHistogramMaxEV100;
        float autoExposureSpeedDarkToBright;
        float autoExposureSpeedBrightToDark;
        int autoExposureUseTargetExposure;
        float dofScale;
        float dofFocalDistance;
        float dofFocalRegion;
        float dofNearTransitionRegion;
        float dofFarTransitionRegion;
        float dofNearRegionBlurSize;
        float dofFarRegionBlurSize;
        float localExposureShadows;
        float localExposureHighlights;
        int32 localExposureCoarsestMipLevel;
        int32 localExposureDisplayMipLevel;
        float localExposurePreferenceSigma;
        float bloomIntensity;
        float bloomRadius;
        float lensDirtIntensity;
        Vector4 lensDirtTint;
        float lensFlaresIntensity;
        float chromaticAberrationIntensity;
        float chromaticAberrationOffset;
        Vector4 colorCorrectionSaturation;
        Vector4 colorCorrectionContrast;
        Vector4 colorCorrectionGamma;
        Vector4 colorCorrectionGain;
        Vector4 colorCorrectionOffset;
        float colorGradingWhiteBalanceColorTemperature;
        bool localExposureEnabled;
    };

    struct RealTimeRendererSceneTextures
    {
        RenderGraphTextureHandle vbuffer0;
        RenderGraphTextureHandle vbuffer1;
        RenderGraphTextureHandle gbuffer0;
        RenderGraphTextureHandle gbuffer1;
        RenderGraphTextureHandle gbuffer2;
        RenderGraphTextureDesc sceneColorTextureDesc;
        RenderGraphTextureHandle sceneColorTexture;
        RenderGraphTextureHandle sceneDepthTexture;
        RenderGraphTextureHandle motionVectorTexture;
        RenderGraphTextureHandle ambientOcclusionTexture;
        RenderGraphTextureHandle hudLessColorTexture;
        RenderGraphTextureHandle uiColorAndAlphaTexture;
        RenderGraphTextureHandle finalColorTexture;
    };
    RENDER_GRAPH_BLACKBOARD_REGISTER_STRUCT(RealTimeRendererSceneTextures);

    struct RealTimeRendererDebugViewModeTextures
    {
        RenderGraphTextureDesc screenSpaceShadowMaskTextureDesc;
        RenderGraphTextureHandle screenSpaceShadowMaskTexture;
    };
    RENDER_GRAPH_BLACKBOARD_REGISTER_STRUCT(RealTimeRendererDebugViewModeTextures);

    struct RealTimeRendererHistoryInfo
    {
        RealTimeRendererSceneViewShaderParameters hisotrySceneViewShaderParameters;
        RenderGraphTextureHandle historySceneDepthTexture;
    };
    RENDER_GRAPH_BLACKBOARD_REGISTER_STRUCT(RealTimeRendererHistoryInfo);

    struct RenderGraphFinalTexture
    {
        RenderGraphTextureDesc finalTextureDesc;
        RenderGraphTextureHandle finalTexture;
    };
    RENDER_GRAPH_BLACKBOARD_REGISTER_STRUCT(RenderGraphFinalTexture);

    struct RenderGraphOutputTexture
    {
        RenderBackendTextureDesc outputTextureDesc;
        RenderGraphTextureHandle outputTexture;
    };
    RENDER_GRAPH_BLACKBOARD_REGISTER_STRUCT(RenderGraphOutputTexture);

    struct RealTimeRendererPreviousSceneView
    {
        float sceneColorPreExposure = 1.0f;
        RenderBackendTextureHandle sceneDepthTexture;
    };

    enum class SuperResolutionTechnique
    {
        None,
        FSR2,
        DLSSSuperResolution,
    };

    enum class FrameRateUpConversionTechnique
    {
        None,
        DLSSFrameGeneration,
    };

    enum class AntialiasingTechnique
    {
        None,
        TemporalAA,
        DLAA,
        FXAA,
    };

    enum class ShadowsTechnique
    {
        None,
        ScreenSpaceShadows,
        RayTracingShadows,
    };

    enum class ReflectionsTechnique
    {
        None,
        ScreenSpaceReflections,
        RayTracingReflections,
    };

    enum class AmbientOcclusionTechnique
    {
        None,
        GroundTruthAmbientOcclusion,
        RayTracingAmbientOcclusion,
    };

    struct GroundTruthAmbientOcclusionSettings
    {
        float radius = 0.2f;
        float factor = 1.0f;
        float thickness = 0.75f;
        bool multiBounce = true;
    };

    enum class ScreenSpaceReflectionsQuality
    {
        Low    = 0,
        Medium = 1,
        High   = 2,
        Epic   = 3,
    };

    struct ScreenSpaceReflectionsSettings
    {
        bool denosingEnabled = true;
        ScreenSpaceReflectionsQuality qualiy = ScreenSpaceReflectionsQuality::Epic;
    };

    enum class FSR2QualityMode
    {
        Custom           = 0,
        Quality          = 1,
        Balanced         = 2,
        Performance      = 3,
        UltraPerformance = 4,
    };

    struct FSR2Settings
    {
        bool useRCAS = false;
        float sharpeness = 1.0f;
        FSR2QualityMode qualityMode = FSR2QualityMode::Custom;
        float customUpscaleRatio = 1.0f;
    };

    enum class PostProcessingLensFlaresQuality
    {
        Disabled,
        Low,
        High,
        VeryHigh,
    };

    enum class DLSSQualityMode
    {
        Off              = 0,
        Auto             = 1,
        Quality          = 2,
        Balanced         = 3,
        Performance      = 4,
        UltraPerformance = 5,
    };

    struct DLSSSettings
    {
        DLSSQualityMode qualityMode = DLSSQualityMode::Auto;
    };

    enum class NVIDIAReflexMode
    {
        Off                 = 0,
        LowLatency          = 1,
        LowLatencyWithBoost = 2,
    };

    enum class ExposureMethod
    {
        FixedExposure,
        AutoExposure,
    };

    enum class RendererType
    {
        RealTime,
        PathTracing,
    };

    enum class ToneMappingOperatorType
    {
        Linear,
        ACES,
        Hable,
        Custom,
    };

    struct RealTimeRendererSettings
    {
        RendererType rendererType;
        bool fixedPreExposureEnabled = false;
        float fixedPreExposure = 1.0f;
        Vector3 indirectLightingTint = Vector3(1.0f, 1.0f, 1.0f);
        float indirectLightingIntensity = 1.0f;
        float upscaleRatio = 1.0f;
        bool forceEnableSubpixelJittering = false;
        bool enableShadowsDenoiser = false;
        ExposureMethod exposureMethod = ExposureMethod::AutoExposure;
        ToneMappingOperatorType toneMappingOperator = ToneMappingOperatorType::ACES;
        ShadowsTechnique shadowsTechnique = ShadowsTechnique::None;
        ReflectionsTechnique reflectionsTechnique = ReflectionsTechnique::None;
        AmbientOcclusionTechnique ambientOcclusionTechnique = AmbientOcclusionTechnique::None;
        AntialiasingTechnique antialiasingTechnique = AntialiasingTechnique::None;
        SuperResolutionTechnique superResolutionTechnique = SuperResolutionTechnique::None;
        ScreenSpaceReflectionsSettings ssrSettings;
        GroundTruthAmbientOcclusionSettings gtaoSettings;
        PostProcessingSettings postProcessingSettings;
        FSR2Settings fsr2Settings;
        DLSSSettings dlssSettings;
        NVIDIAReflexMode reflexMode = NVIDIAReflexMode::LowLatency;
    };

    class RealTimeRenderer : public RenderPipeline
    {
    public:

        RealTimeRenderer(RenderBackend* backend, ShaderCompiler* compiler, RenderSystem* renderEngine);
        virtual ~RealTimeRenderer();

        void UpdatePerFrameData(const SceneView& view, RenderBackendCommandList* commandList);

        void SetupRenderGraph(RenderGraph& renderGraph, const SceneView& view) override;

        RealTimeRendererSettings settings;

    private:

        bool IsSurfelGIEnabled() const
        {
            return isSurfelGIEnabled;
        }

        bool IsRayTracingShadowsEnabled() const
        {
            return isRayTracingShadowsEnabled;
        }

        bool IsRayTracingReflectionsEnabled() const
        {
            return isRayTracingReflectionsEnabled;
        }

        bool IsRayTracingAmbientOcclusionEnabled() const
        {
            return isRayTracingAmbientOcclusionEnabled;
        }

        bool IsDLAAEnabled() const
        {
            return isDLAAEnabled;
        }

        bool IsDLSSEnabled() const
        {
            return isDLSSEnabled;
        }

        bool IsTemporalAAEnabled() const
        {
            return isTemporalAAEnabled;
        }

        bool IsFSR2Enabled() const
        {
            return isFSR2Enabled;
        }

        bool IsSuperResolutionEnabled() const
        {
            return isSuperResolutionEnabled;
        }

        bool ShouldApplyCameraJittering() const
        {
            if (settings.forceEnableSubpixelJittering)
            {
                return true;
            }
            if (IsTemporalAAEnabled() ||
                IsDLAAEnabled() ||
                IsDLSSEnabled() ||
                IsFSR2Enabled())
            {
                return true;
            }
            return false;
        }

        bool ShouldRenderSkyAtmosphere() const
        {
            if (skyAtmosphere && renderEngine->skyAtmosphereComponent)
            {
                return true;
            }
            return false;
        }

        bool LoadShaders();

        bool GatherRayTracingInstances(
            RenderGraph& renderGraph,
            SceneView& view,
            RayTracingScene& rayTracingScene);

        void AddSurfleGIPasses(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle AddSurfleGIVisualizationPass(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderVisibilityBuffer(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderGBuffer(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderMotionVectors(
            RenderGraph& renderGraph,
            const SceneView& view);

        void ComputeLocalLightCulling(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderScreenSpaceShadows(
            RenderGraph& renderGraph,
            const SceneView& view,
            const LightComponent& light,
            RenderGraphTextureHandle& screenSpaceShadowMaskTexture);

        void RenderRayTracingShadows(
            RenderGraph& renderGraph,
            const SceneView& view,
            const LightComponent& light,
            RenderGraphTextureHandle& screenSpaceShadowMaskTexture,
            RenderGraphTextureHandle& rayDistanceTexture);

        RenderGraphTextureHandle RenderLocalLightShadows(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderScreenSpaceReflections(
            RenderGraph& renderGraph,
            const SceneView& view,
            uint32 hzbWidth,
            uint32 hzbHeight,
            RenderGraphTextureHandle hzb,
            RenderGraphTextureHandle historySceneColor,
            RenderGraphTextureHandle historySceneDepth,
            RenderBackendBufferHandle rayAllocationBuffer,
            RenderGraphTextureHandle& ssrTexture,
            RenderGraphTextureHandle& debugOutputTexture);

        RenderGraphTextureHandle RenderScreenSpaceAmbientOcclusion(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle RenderRayTracingAmbientOcclusion(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderHZB(
            RenderGraph& renderGraph,
            const SceneView& view,
            uint32 hzbWidth,
            uint32 hzbHeight,
            uint32 hzbMipLevels,
            RenderGraphTextureHandle& closestHZBTexture,
            RenderGraphTextureHandle& furthestHZBTexture);

        void AddDirectLightingPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle screenSpaceShadowMaskTexture,
            RenderGraphTextureHandle localLightShadowMapAtlas);

        void AddIndirectLightingDiffusePass(
            RenderGraph& renderGraph,
            const SceneView& view);

        void AddIndirectLightingSpecularPass(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderSubsurfaceScattering(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderLightShafts(
            RenderGraph& renderGraph,
            const SceneView& view);

        void AddPostProcessingPasses(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle AddDepthOfFieldPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColor);

        RenderGraphTextureHandle AddAutoExposureBuildHistogramPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture);

        RenderGraphBufferHandle AddAutoExposureComputeExposurePass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle autoExposureHistogramTexture,
            RenderGraphBufferHandle historyAutoExposureBuffer);

        RenderGraphTextureHandle AddLocalExposurePass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture,
            RenderGraphTextureHandle autoExposureTexture);

        RenderGraphTextureHandle AddColorLUTPass(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle AddToneMappingPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture,
            RenderGraphTextureHandle bloomTexture,
            RenderGraphBufferHandle autoExposureBuffer,
            RenderGraphTextureHandle colorLUTTexture,
            RenderGraphTextureHandle localExposureTexture,
            bool outputInHDR);

        RenderGraphTextureHandle AddTemporalSuperSamplingPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture,
            RenderGraphTextureHandle sceneDepthTexture,
            RenderGraphTextureHandle motionVectorTexture);

        RenderGraphTextureHandle AddFSR2Pass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture,
            RenderGraphTextureHandle sceneDepthTexture,
            RenderGraphTextureHandle motionVectorTexture);

        RenderGraphTextureHandle AddDLSSPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture,
            RenderGraphTextureHandle sceneDepthTexture,
            RenderGraphTextureHandle motionVectorTexture);

        //RenderGraphTextureHandle AddFXAAPass(
        //    RenderGraph& renderGraph,
        //    const SceneView& view,
        //    RenderGraphTextureHandle sceneColor);

        //void DenoiseShadowMaskSSD(
        //    RenderGraph& renderGraph,
        //    const SceneView& view,
        //    RenderGraphTextureHandle& filteredShadowMask,
        //    RenderGraphTextureHandle& shadowMask);

        RenderGraphTextureHandle AddMotionBlurPass(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle AddLensFlaresPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle halfResolutionSceneColorTexture,
            RenderGraphTextureHandle bloomTexture);

        RenderGraphTextureHandle AddGaussianBloomPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle halfResolutionSceneColorTexture);

        RenderGraphTextureHandle AddConvolutionBloomPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColor,
            RenderGraphTextureHandle autoExposureTexture);

        RenderGraphTextureHandle AddDownsamplePass(
            RenderGraph& renderGraph,
            const SceneView& view,
            uint32 inputTextureWidth,
            uint32 inputTextureHeight,
            uint32 outputTextureWidth,
            uint32 outputTextureHeight,
            RenderGraphTextureHandle inputTexture,
            RenderGraphTextureHandle outputTexture);

        void AddGenerateSceneColorMipChainPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture,
            PostProcessingSceneColorMipChain* outMipChain);

        RenderGraphTextureHandle AddEditorSelectionOutlinePass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture);

        RenderGraphTextureHandle AddVisualizePrimitiveIDPass(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle AddVisualizeMaterialIDPass(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle AddVisualizeWorldSpaceNormalPass(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle AddVisualizeMotionVectorsPass(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle AddVisualizeAmbientOcclusionPass(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle AddVisualizeShadowMaskPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture);

        RenderGraphTextureHandle RenderUIColorAndAlpha(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderBackend* renderBackend;
        ShaderCompiler* shaderCompiler;
        ShaderLibrary_Deprecated* shaderLibrary;
        RenderSystem* renderEngine;

        RenderBackendSamplerHandle samplerLinearWarp;
        RenderBackendSamplerHandle samplerLinearClamp;
        RenderBackendSamplerHandle samplerLinearBorder;
        RenderBackendSamplerHandle samplerPointWarp;
        RenderBackendSamplerHandle samplerPointClamp;
        RenderBackendSamplerHandle samplerPointBorder;
        RenderBackendSamplerHandle samplerComparisonGreaterLinearClamp;
        RenderBackendSamplerHandle samplerCmpDepth;

        RenderBackendColorBlendAttachmentState additiveRGBAColorBlendAttachmentState;
        RenderBackendColorBlendAttachmentState additiveRGBColorBlendAttachmentState;

        RenderBackendTextureHandle testTexture;

        RenderBackendTextureHandle lensDirtTexture;
        RenderBackendTextureHandle lensFlaresGlareLUTTexture;
        RenderBackendTextureHandle lensFlaresGradiantLUTTexture;

        RenderBackendTextureHandle blueNoiseTexture;

        RenderBackendTextureDesc defaultBloomKernelTextureDesc;
        RenderBackendTextureHandle defaultBloomKernelTexture;

        RenderBackendTextureDesc localExposureTestTextureDesc;
        RenderBackendTextureHandle localExposureTestTexture;

        static const uint32 NumAutoExposureReadbackBuffers = 4;
        RenderGraphPersistentBuffer autoExposureReadbackBuffers[NumAutoExposureReadbackBuffers];
        int32 currentAutoExposureReadbackBufferIndex = 0;

        float GetHistoryExposureFromAutoExposureReadbackBuffer();

        RealTimeRendererSceneViewShaderParameters sceneViewShaderParameters;
        RenderBackendBufferHandle sceneViewShaderParametersBuffer;
        RenderBackendBufferHandle sceneViewShaderParametersUploadBuffer;

        RenderBackendRayTracingPipelineStateHandle rayTracingShadowsPipelineState;
        RenderBackendBufferHandle rayTracingShadowsSBT;

        RenderBackendBufferHandle ssrRayAllocationBuffer;

        RenderBackendBufferHandle surfelGIInfoBuffer;
        RenderBackendBufferHandle surfelGIArgumentBuffer;
        RenderBackendBufferHandle surfelGIAliveSurfelIndirectionBuffer;
        RenderBackendBufferHandle surfelGIFreeSurfelBuffer;
        RenderBackendBufferHandle surfelGISurfelHotDataBuffer;
        RenderBackendBufferHandle surfelGICellHeaderBuffer;
        RenderBackendBufferHandle surfelGICellDataBuffer;

        RenderGraphPersistentTexture historySceneColorTextureCache;
        RenderGraphPersistentTexture historySceneDepthTextureCache;
        RenderGraphPersistentTexture historyAmbientOcclusionTextureCache;
        RenderGraphPersistentTexture historyTemporalSuperSamplingTextureCache;
        RenderGraphPersistentTexture ssrTemporalFilteringOutputTextureCache;
        RenderGraphPersistentTexture ssrTemporalVarianceTextureCache;

        RenderGraphPersistentBuffer historyAutoExposureBufferPersistent;

        SkyAtmosphere* skyAtmosphere = nullptr;

        bool isTemporalAAEnabled = false;

        bool isDLAAEnabled = false;
        bool isDLSSEnabled = false;
        bool isFSR2Enabled = false;
        bool isSuperResolutionEnabled = false;
        bool isSurfelGIEnabled = false;
        bool isRayTracingShadowsEnabled = false;;
        bool isRayTracingReflectionsEnabled = false;
        bool isRayTracingAmbientOcclusionEnabled = false;

        uint32 renderResolutionX;
        uint32 renderResolutionY;
        uint32 targetResolutionX;
        uint32 targetResolutionY;

        float upscaleRatio = 1.0f;

        float NearClipPlaneDepthValue = 1.0f;
        float FarClipPlaneDepthValue = 0.0f;
    };
}