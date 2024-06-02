#pragma once

#include "RealTimeRendererCommon.h"
#include "PostProcessing/PostProcessing.h"

namespace Horizon
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

    struct RealTimeRendererSceneTextures
    {
        RenderGraphTextureHandle vbuffer0;
        RenderGraphTextureHandle vbuffer1;
        RenderGraphTextureHandle gbuffer0;
        RenderGraphTextureHandle gbuffer1;
        RenderGraphTextureHandle gbuffer2;
        RenderGraphTextureHandle depthPyramidTexture;
        RenderGraphTextureHandle sceneColorTexture;
        RenderGraphTextureHandle sceneDepthTexture;
        RenderGraphTextureHandle motionVectorTexture;
        RenderGraphTextureHandle ambientOcclusionTexture;
        RenderGraphTextureHandle hudLessColorTexture;
        //RenderGraphTextureHandle uiColorAndAlphaTexture;
        //RenderGraphTextureHandle targetTexture;
        //RenderGraphTextureHandle displayTexture;
    };

    // struct RealTimeRendererDebugViewModeTextures
    // {
    //     RenderGraphTextureDesc screenSpaceShadowMaskTextureDesc;
    //     RenderGraphTextureHandle screenSpaceShadowMaskTexture;
    // };
    // static const RenderGraphBlackboardRegistry<RealTimeRendererDebugViewModeTextures> RealTimeRendererSceneTexturesRegistry;
    //
    // struct RenderGraphFinalTexture
    // {
    //     RenderGraphTextureDesc finalTextureDesc;
    //     RenderGraphTextureHandle finalTexture;
    // };
    // static const RenderGraphBlackboardRegistry<RenderGraphFinalTexture> RealTimeRendererSceneTexturesRegistry;

    // struct RenderGraphOutputTexture
    // {
    //     RenderBackendTextureDesc outputTextureDesc;
    //     RenderGraphTextureHandle outputTexture;
    // };
    // static const RenderGraphBlackboardRegistry<RenderGraphOutputTexture> RealTimeRendererSceneTexturesRegistry;

    class RealTimeRenderer// : public SceneRenderer
    {
    public:

        RealTimeRenderer(
            RenderBackend* renderBackend,
            RenderGraphResourcePool* resourcePool,
            ShaderLibrary* shaderLibrary,
            RendererDefaultResources* defaultResources);

        virtual ~RealTimeRenderer();

        void Render(RenderGraph& renderGraph);

        bool IsSuperResolutionEnabled() const;

        bool IsAutoExposureEnabled() const;

        bool IsLocalExposureEnabled() const;

        bool IsSkyAtmosphereRenderingEnabled() const;

        bool IsSkyAtmosphereDebugVisualizationEnabled() const;

        bool IsSubsurfaceScatteringEnabled() const;

        bool IsSurfelGIEnabled() const;

        bool IsScreenSpaceShadowsEnabled() const;

        bool IsScreenSpaceReflectionsEnabled() const;

        bool IsScreenSpaceAmbientOcclusionEnabled() const;

        bool IsScreenSpaceLightShaftsEnabled() const;

        //bool IsRayTracingShadowsEnabled() const;

        //bool IsRayTracingReflectionsEnabled() const;

        //bool IsRayTracingAmbientOcclusionEnabled() const;

        bool IsMotionBlurEnabled() const;

        bool IsDepthOfFieldEnabled() const;

        bool IsGaussianBloomEnabled() const;

        bool IsConvolutionBloomEnabled() const;

        bool IsBloomEnabled() const;

        bool IsLensFlaresEnabled() const;

        void OnRenderBegin(SceneView* sceneView);

    private:

        void UpdatePerFrameDataBuffer() const;

        // bool ShouldApplyCameraJittering() const
        // {
        //     if (settings.forceEnableSubpixelJittering)
        //     {
        //         return true;
        //     }
        //     if (IsTemporalAAEnabled() ||
        //         IsDLAAEnabled() ||
        //         IsDLSSEnabled() ||
        //         IsFSR2Enabled())
        //     {
        //         return true;
        //     }
        //     return false;
        // }

        bool LoadShaders();

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

        void DispatchLocalLightCulling(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderScreenSpaceShadows(
            RenderGraph& renderGraph,
            const SceneView& view,
            const LightRenderObject& light,
            RenderGraphTextureHandle& screenSpaceShadowMaskTexture);

        void RenderRayTracingShadows(
            RenderGraph& renderGraph,
            const SceneView& view,
            const LightRenderObject& light,
            RenderGraphTextureHandle& screenSpaceShadowMaskTexture,
            RenderGraphTextureHandle& rayDistanceTexture);

        RenderGraphTextureHandle RenderLocalLightShadows(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderScreenSpaceReflections(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle RenderScreenSpaceAmbientOcclusion(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle RenderRayTracingAmbientOcclusion(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderDepthPyramid(
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

        void RenderSkyAtmosphereLUTs(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderSkyAtmosphere(
            RenderGraph& renderGraph,
            const SceneView& view);

        void AddSkyAtmosphereDebugVisualizationPass();

        void RenderSubsurfaceScattering(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderScreenSpaceLightShafts(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderPostProcessingEffects(
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
            RenderGraphBufferHandle previousAutoExposureBuffer);

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
            RenderGraphTextureHandle colorLUTTexture,
            RenderGraphTextureHandle localExposureTexture,
            RenderGraphBufferHandle autoExposureBuffer,
            bool outputInHDR);

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

        RenderGraphTextureHandle DispatchGaussianBloom(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle halfResolutionSceneColorTexture);

        RenderGraphTextureHandle DispatchConvolutionBloom(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture);

        RenderGraphTextureHandle AddDownsamplePass(
            RenderGraph& renderGraph,
            const SceneView& view,
            uint32 inputTextureWidth,
            uint32 inputTextureHeight,
            uint32 outputTextureWidth,
            uint32 outputTextureHeight,
            RenderGraphTextureHandle inputTexture,
            RenderGraphTextureHandle outputTexture);

        void RenderSceneColorPyramid(
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

        RenderGraphTextureHandle RenderUserInterface(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderBackend* renderBackend;
        RenderGraphResourcePool* resourcePool;
        ShaderLibrary* shaderLibrary;
        RendererDefaultResources* defaultResources;
        SceneView* sceneView;

        struct RenderFeatures
        {
            uint32 enableFrameRateUpConversion : 1;
            uint32 enableSuperSamplingAntiAliasing : 1;
            uint32 enableSuperResolution : 1;
            uint32 enableSkyAtmosphereRendering : 1;
            uint32 enableSubsurfaceScattering : 1;
            uint32 enableScreenSpaceShadows : 1;
            uint32 enableScreenSpaceReflections : 1;
            uint32 enableScreenSpaceAmbientOcclusion : 1;
            uint32 enableScreenSpaceLightShafts : 1;
            uint32 enableRayTracingShadows : 1;
            uint32 enableRayTracingReflections : 1;
            uint32 enableRayTracingAmbientOcclusion : 1;
            uint32 enableSurfelGI : 1;
            uint32 enableMotionBlur : 1;
            uint32 enableAutoExposure : 1;
            uint32 enableLocalExposure : 1;
            uint32 enableDepthOfField : 1;
            uint32 enableLensFlares : 1;
            uint32 enableGaussianBloom : 1;
            uint32 enableConvolutionBloom : 1;
        } features;

        float upscaleRatio = 1.0f;

        Extent2D renderResolution;
        Extent2D targetResolution;
        Extent2D displayResolution;

        PostProcessingSettings finalPostProcessingSettings;

        static const int32 MaxNumFramesInFlight = 3;
        int32 currentPerFrameDataBufferIndex = 0;
        RenderBackendBufferHandle perFrameDataBuffers[MaxNumFramesInFlight];

        RenderBackendBufferHandle GetCurrentPerFrameDataBuffer() const;

        float materialTextureMipLodBias;

        float preExposure = 1.0f;

        struct AutoExposureData
        {
            float adaptedExposure;
            float targetExposure;
            float exposureCompensation;
            float averageSceneLuminance;
        };
        static const int32 NumAutoExposureReadbackBuffers = 4;
        int32 currentAutoExposureReadbackBufferIndex = 0;
        RenderGraphPersistentBuffer* autoExposureReadbackBuffers[NumAutoExposureReadbackBuffers];

        AutoExposureData autoExposureData;

        void UpdateAutoExposureDataFromReadbackBuffer();

        struct HistoryFrame
        {
            Vector3 cameraPosition;
            Vector2 cameraJitterOffset;
            CameraTransformations transformations;
            float preExposure;
            RenderGraphPersistentBuffer* autoExposureBuffer;
            RenderGraphPersistentTexture* sceneDepthTexture;
            RenderGraphPersistentTexture* ambientOcclusionTexture;
            RenderGraphPersistentTexture* temporalSuperSamplingTexture;
        };

        HistoryFrame historyFrame;

        void ResetHistoryFrame();
    };
}