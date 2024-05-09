#pragma once

#include "Rendering/ShaderID_DEPRECATED.h"
#include "Rendering/Renderer/RendererInterface.h"
#include "Rendering/Renderer/RealTimeRenderer/RealTimeRendererCommon.h"
#include "Rendering/Renderer/RealTimeRenderer/PostProcessing/PostProcessing.h"
#include "Rendering/Renderer/RealTimeRenderer/SkyAtmosphereRendering.h"

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
        RenderGraphTextureHandle sceneColorTexture;
        RenderGraphTextureHandle sceneDepthTexture;
        RenderGraphTextureHandle motionVectorTexture;
        RenderGraphTextureHandle ambientOcclusionTexture;
        RenderGraphTextureHandle hudLessColorTexture;
        RenderGraphTextureHandle uiColorAndAlphaTexture;
        RenderGraphTextureHandle finalColorTexture;
    };
    static const RenderGraphBlackboardRegistry<RealTimeRendererSceneTextures> RealTimeRendererSceneTexturesRegistry;

    struct RealTimeRendererDebugViewModeTextures
    {
        RenderGraphTextureDesc screenSpaceShadowMaskTextureDesc;
        RenderGraphTextureHandle screenSpaceShadowMaskTexture;
    };
    static const RenderGraphBlackboardRegistry<RealTimeRendererDebugViewModeTextures> RealTimeRendererSceneTexturesRegistry;

    struct RenderGraphFinalTexture
    {
        RenderGraphTextureDesc finalTextureDesc;
        RenderGraphTextureHandle finalTexture;
    };
    static const RenderGraphBlackboardRegistry<RenderGraphFinalTexture> RealTimeRendererSceneTexturesRegistry;

    struct RenderGraphOutputTexture
    {
        RenderBackendTextureDesc outputTextureDesc;
        RenderGraphTextureHandle outputTexture;
    };
    static const RenderGraphBlackboardRegistry<RenderGraphOutputTexture> RealTimeRendererSceneTexturesRegistry;

    class RealTimeRenderer : public SceneRenderer
    {
    public:

        RealTimeRenderer(SceneView* view);
        virtual ~RealTimeRenderer();

        void Render(RenderGraph& renderGraph) override;

    private:

        void UpdatePerFrameData();

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

        bool IsSkyAtmosphereRenderingEnabled() const;

        void RenderSkyAtmosphereLUTs(RenderGraph& renderGraph);

        void RenderSkyAtmosphere(RenderGraph& renderGraph);

        bool IsSkyAtmosphereDebugVisualizationEnabled() const;

        void AddSkyAtmosphereDebugVisualizationPass();

        bool isSkyAtmosphereRenderingEnabled = false;

        bool isSkyAtmosphereDebugVisualizationEnabled = false;

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

        void DispatchLocalLightCulling(
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

        void RenderScreenSpaceLightShafts(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderPostProcessingEffects(
            RenderGraph& renderGraph,
            const SceneView& view);

        [[deprecated]]
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
        ShaderLibrary_DEPRECATED* shaderLibrary;
        RenderSystem* renderSystem;
        RenderGraphResourcePool* resourcePool;
        RendererDefaultResources* defaultResources;
        RenderScene* scene;

        bool isDLAAEnabled = false;
        bool isDLSSEnabled = false;
        bool isFSR2Enabled = false;
        bool isTemporalAAEnabled = false;
        bool isSuperResolutionEnabled = false;
        bool isSurfelGIEnabled = false;
        bool isRayTracingShadowsEnabled = false;;
        bool isRayTracingReflectionsEnabled = false;
        bool isRayTracingAmbientOcclusionEnabled = false;
        bool isScreenSpaceLightShaftsEnabled = false;

        bool isMotionBlurEnabled = false;
        bool isAutoExposureEnabled = (settings.exposureMethod == ExposureMethod::AutoExposure) || (settings.exposureMethod == ExposureMethod::FixedExposure);
        bool isBloomEnabled = settings.postProcessingSettings.bloomIntensity > 0.0f;
        bool isLensFlaresEnabled = isBloomEnabled && settings.postProcessingSettings.lensFlaresIntensity > 0.0f;
        bool isConvolutionBloomEnabled = false;
        bool isToneMappingEnabled = true;
        bool isLocalExposureEnabled = isToneMappingEnabled && settings.postProcessingSettings.localExposureEnabled;

        float upscaleRatio = 1.0f;
        Extent2D renderResolution;
        Extent2D targetResolution;
        Extent2D displayResolution;

        const float NearClippingPlaneDepthValue = 1.0f;
        const float FarClippingPlaneDepthValue = 0.0f;

        static const uint32 MaxNumFramesInFlight = 3;
        int32 currentPerFrameDataBufferIndex = 0;
        RenderBackendBufferHandle perFrameDataBuffers[MaxNumFramesInFlight];

        RenderBackendBufferHandle GetCurrentPerFrameDataBuffer();

        float preExposure = 1.0f;
        float previousPreExposure = 1.0f;

        RenderGraphPersistentBuffer* autoExposureBufferHistory = nullptr;

        struct AutoExposureData
        {
            float adaptedExposure;
            float targetExposure;
            float exposureCompensation;
            float averageSceneLuminance;
        };
        static const uint32 NumAutoExposureReadbackBuffers = 4;
        int32 currentAutoExposureReadbackBufferIndex = 0;
        RenderGraphPersistentBuffer* autoExposureReadbackBuffers[NumAutoExposureReadbackBuffers];

        AutoExposureData autoExposureData;

        void UpdateAutoExposureDataFromReadbackBuffer();

        RenderGraphPersistentTexture* sceneDepthTextureHistory = nullptr;
        RenderGraphPersistentTexture* ambientOcclusionTextureHistory = nullptr;
        RenderGraphPersistentTexture* temporalSuperSamplingTextureHistory = nullptr;
    };
}