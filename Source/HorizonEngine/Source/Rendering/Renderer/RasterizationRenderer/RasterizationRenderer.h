#pragma once

#include "RasterizationRendererCommon.h"
#include "GeometryRendering.h"
#include "ManyLightRendering.h"
#include "PostProcessing/PostProcessing.h"
#include "PerFrameShaderParameters.h"
#include "ShadowMapping.h"
#include "VirtualShadowMaps.h"

// todo
#define NEAR_CLIPPING_PLANE_DEPTH_VALUE 1.0f
#define FAR_CLIPPING_PLANE_DEPTH_VALUE 0.0f

namespace Horizon
{
    class TemporalSuperSamplingInterface;

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

    static constexpr uint32 IndexCountPerMeshlet = 128;

    // Surfel hot data
    struct SurfelHotData
    {
        Vector3f position;
        Vector3f normal;
        float radius;
    };

    // Surfel cold data
    struct SurfelColdData
    {
        Vector3f position;
        Vector3f normal;
        Vector2i primitiveID;
    };

    struct CellHeader
    {
        uint32 capacity;
        uint32 offset;
    };

    struct DistantLightShaderParameters
    {
        Vector3f direction;
        Vector3f tangent;
        Vector3f color;
    };

    struct DistantLightRenderData
    {
        CascadedShadowMapRenderData* cascadedShadowMapRenderData;
        //VirtualShadowMapClipmapRenderData* virtualShadowMapClipmapRenderData;
    };

    struct LocalLightRenderData
    {

    };

    struct RasterizationRendererIntermediateResources
    {
        RenderGraphBufferHandle visibleMeshletBuffer;
        RenderGraphTextureHandle vbuffer0;
        RenderGraphTextureHandle vbuffer1;
        RenderGraphTextureHandle gbuffer0;
        RenderGraphTextureHandle gbuffer1;
        RenderGraphTextureHandle gbuffer2;
        RenderGraphTextureHandle colorTexture;
        RenderGraphTextureHandle depthTexture;
        RenderGraphTextureHandle minDepthPyramidTexture;
        RenderGraphTextureHandle maxDepthPyramidTexture;
        RenderGraphTextureHandle motionVectorTexture;
        RenderGraphTextureHandle ambientOcclusionTexture;
        RenderGraphTextureHandle indirectDiffuseTexture;
        RenderGraphTextureHandle screenSpaceReflectionTexture;
        RenderGraphTextureHandle hudLessColorTexture;
        RenderGraphTextureHandle environmentMapTexture;
        RenderGraphBufferHandle irradianceEnvironmentMapBuffer;
        RenderGraphTextureHandle convolvedEnvironmentMapTexture;
        RenderGraphBufferHandle virtualShadowMapShaderParameterBuffer;
        RenderGraphBufferHandle virtualShadowMapPageTableBuffer;
        RenderGraphBufferHandle virtualShadowMapEntryBuffer;
        RenderGraphTextureHandle virtualShadowMapDepthTexture;
        RenderGraphTextureHandle virtualShadowMapDebugVisualizationTexture;
        RenderBackendBufferHandle cascadedShadowMapShaderParameterBuffer;
        RenderGraphTextureHandle cascadedShadowMapDepthTexture;
        RenderGraphTextureHandle cascadedShadowMapDebugVisualizationTexture;
        RenderGraphTextureHandle shadowMaskTexture;
    };

    struct RasterizationRendererLightGridData
    {
        LightGridInfo lightGridInfo;
        RenderGraphBufferHandle lightGridCellDataBuffer;
        RenderGraphBufferHandle lightGridLightListBuffer;
    };

    // struct RasterizationRendererDebugViewModeTextures
    // {
    //     RenderGraphTextureDesc screenSpaceShadowMaskTextureDesc;
    //     RenderGraphTextureHandle screenSpaceShadowMaskTexture;
    // };
    // static const RenderGraphBlackboardRegistry<RasterizationRendererDebugViewModeTextures> RasterizationRendererSceneTexturesRegistry;
    //
    // struct RenderGraphFinalTexture
    // {
    //     RenderGraphTextureDesc finalTextureDesc;
    //     RenderGraphTextureHandle finalTexture;
    // };
    // static const RenderGraphBlackboardRegistry<RenderGraphFinalTexture> RasterizationRendererSceneTexturesRegistry;

    // struct RenderGraphOutputTexture
    // {
    //     RenderBackendTextureDesc outputTextureDesc;
    //     RenderGraphTextureHandle outputTexture;
    // };
    // static const RenderGraphBlackboardRegistry<RenderGraphOutputTexture> RasterizationRendererSceneTexturesRegistry;

    class RasterizationRenderer// : public SceneRenderer
    {
    public:

        RasterizationRenderer(
            RenderBackend* renderBackend,
            RenderGraphResourcePool* resourcePool,
            ShaderCollection* shaderLibrary,
            RendererDefaultResources* defaultResources);

        virtual ~RasterizationRenderer();

        void Render(RenderGraph& renderGraph);

        bool IsSuperResolutionEnabled() const;

        bool IsLocalToneMappingEnabled() const;

        bool IsSkyAtmosphereDebugVisualizationEnabled() const;

        bool IsSurfelGIEnabled() const;

        bool IsScreenSpaceShadowsEnabled() const;

        bool IsScreenSpaceReflectionsEnabled() const;

        //bool IsRayTracingShadowsEnabled() const;

        //bool IsRayTracingReflectionsEnabled() const;

        //bool IsRayTracingAmbientOcclusionEnabled() const;

        bool IsDepthOfFieldEnabled() const;

        bool IsGaussianBloomEnabled() const;

        bool IsConvolutionBloomEnabled() const;

        bool IsLensFlareEnabled() const;

        void InitializeSceneView(SceneView* sceneView);

    private:

        void UpdatePerFrameDataBuffer();

        void GatherVisibleLights();

        void CreateDynamicShadowData();

        void DispatchDynamicShadowSetupJobs();

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

        void DispatchPathTracing(RenderGraph& renderGraph, const SceneView& view);

        void AddSurfleGIPasses(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle AddSurfleGIVisualizationPass(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderVisibilityBuffer(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderVisibilityBufferMeshShading(
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

        void RenderShadowMapDepth(
            RenderGraph& renderGraph,
            const SceneView& view);

        void DispatchShadowMapProjection(
            RenderGraph& renderGraph,
            const SceneView& view);

        void DispatchScreenSpaceShadows(
            RenderGraph& renderGraph,
            const SceneView& view,
            const LightRenderObject& light);

        void RenderVirtualShadowMapDepth(
            RenderGraph& renderGraph,
            const SceneView& view);

        void DispatchVirtualShadowMapProjection(
            RenderGraph& renderGraph,
            const SceneView& view);

        void DispatchRayTracingShadows(
            RenderGraph& renderGraph,
            const SceneView& view,
            const LightRenderObject& light,
            RenderGraphTextureHandle& screenSpaceShadowMaskTexture,
            RenderGraphTextureHandle& rayDistanceTexture);

        RenderGraphTextureHandle RenderLocalLightShadows(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderScreenSpaceIndirectDiffuse(
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

        void DispatchDepthPyramidGeneration(
            RenderGraph& renderGraph,
            const SceneView& view);

        void AddDirectLightingPass(
            RenderGraph& renderGraph,
            const SceneView& view,
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

        void CaptureEnvironmentMap(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderSubsurfaceScattering(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderScreenSpaceLightShafts(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderVolumetricFog(
            RenderGraph& renderGraph,
            const SceneView& view);

        void RenderLocalFogVolumes(
            RenderGraph& renderGraph,
            const SceneView& view);

        void ExecutePostProcessingPipeline(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle DispatchDepthOfField(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColor);

        RenderGraphBufferHandle DispatchHistogramBasedAutoExposure(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle colorTexture,
            const PostProcessingColorPyramid& colorPyramid,
            RenderGraphBufferHandle previousAutoExposureBuffer);

        RenderGraphTextureHandle AddCopyExposurePass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphBufferHandle autoExposureBuffer);

        RenderGraphTextureHandle DispatchBilateralGridLocalToneMapping(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle colorTexture,
            const PostProcessingColorPyramid& colorPyramid,
            RenderGraphBufferHandle autoExposureBuffer);

        RenderGraphTextureHandle DispatchExposureFusionLocalToneMapping(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle colorTexture,
            const PostProcessingColorPyramid& colorPyramid,
            RenderGraphTextureHandle exposureTexture);

        RenderGraphTextureHandle RenderColorTransformLUT(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle DispatchFinalComposition(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle colorTexture,
            RenderGraphTextureHandle bloomTexture,
            RenderGraphTextureHandle localToneMappingTexture,
            RenderGraphTextureHandle colorTransformLUTTexture,
            RenderGraphBufferHandle autoExposureBuffer);

        //void DenoiseShadowMaskSSD(
        //    RenderGraph& renderGraph,
        //    const SceneView& view,
        //    RenderGraphTextureHandle& filteredShadowMask,
        //    RenderGraphTextureHandle& shadowMask);

        RenderGraphTextureHandle DispatchMotionBlur(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle colorTexture,
            RenderGraphTextureHandle depthTexture,
            RenderGraphTextureHandle velocityTexture);

        RenderGraphTextureHandle DispatchLensFlarePass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle colorTexture,
            const PostProcessingColorPyramid& colorPyramid,
            RenderGraphTextureHandle bloomTexture);

        RenderGraphTextureHandle DispatchGaussianBloom(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle colorTexture,
            const PostProcessingColorPyramid& colorPyramid);

        RenderGraphTextureHandle DispatchConvolutionBloom(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle colorTexture,
            const PostProcessingColorPyramid& colorPyramid);

        RenderGraphTextureHandle AddDownsamplePass(
            RenderGraph& renderGraph,
            const SceneView& view,
            uint32 inputTextureWidth,
            uint32 inputTextureHeight,
            uint32 outputTextureWidth,
            uint32 outputTextureHeight,
            RenderGraphTextureHandle inputTexture,
            RenderGraphTextureHandle outputTexture);

        void DispatchColorPyramidGeneration(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture,
            PostProcessingColorPyramid* outMipChain);

        RenderGraphTextureHandle DispatchEditorSelectionOutline(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture);

        RenderGraphTextureHandle DispatchDepthDebugVisualization(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle DispatchVirtualGeometryDebugVisualization(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle DispatchWorldSpaceNormalDebugVisualization(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle DispatchMotionVectorDebugVisualization(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle DispatchAmbientOcclusionDebugVisualization(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle AddVisualizeShadowMaskPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture);

        RenderGraphTextureHandle DispatchCascadedShadowMapDebugVisualization(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle DispatchVirtualShadowMapDebugVisualization(
            RenderGraph& renderGraph,
            const SceneView& view);

        RenderGraphTextureHandle AddDebugDrawPass(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColorTexture,
            RenderGraphTextureHandle sceneDepthTexture);

        void SetupGeometryPasses();

        RenderBackend* renderBackend;
        RenderGraphResourcePool* resourcePool;
        ShaderCollection* shaderCollection;
        RendererDefaultResources* defaultResources;
        SceneView* sceneView;
        TemporalSuperSamplingInterface* temporalSuperSamplingInterface;
        RasterizationRendererSettings rendererSettings;

        struct RenderFeatures
        {
            bool enableFrameRateUpConversion;
            bool enableTemporalSuperSampling;
            bool enableSuperSamplingAntiAliasing;
            bool enableSkyAtmosphereRendering;
            bool enableSubsurfaceScattering;
            bool enableVolumetricFog;
            bool enableLocalFogVolume;
            bool enableScreenSpaceShadows;
            bool enableScreenSpaceReflections;
            bool enableScreenSpaceAmbientOcclusion;
            bool enableScreenSpaceLightShafts;
            bool enableRayTracingShadows;
            bool enableRayTracingReflections;
            bool enableRayTracingAmbientOcclusion;
            bool enableSurfelGI;
            bool enableMotionBlur;
            bool enableAutoExposure;
            bool enableBilateralGridLocalToneMapping;
            bool enableExposureFusionLocalToneMapping;
            bool enableDepthOfField;
            bool enableLensFlare;
            bool enableGaussianBloom;
            bool enableConvolutionBloom;
#if HORIZON_EDITOR
            bool enableEditorSelectionOutline;
#endif
        } renderFeatures;

        float renderResolutionPercentage = 1.0f;

        Extent2D renderResolution;
        Extent2D targetResolution;
        Extent2D displayResolution;

        Vector2f cameraJitterOffset;

        Matrix4x4f reprojectionMatrix;
        Matrix4x4f inverseReprojectionMatrix;

        RasterizationRendererPostProcessingSettings finalPostProcessingSettings;

        PerFrameShaderParameters perFrameShaderParameters = {};

        static const int32 MaxNumFramesInFlight = 3;

        int32 currentPerFrameDataBufferIndex = 0;
        RenderBackendBufferHandle currentPerFrameConstantBuffer;

        RenderBackendBufferHandle perFrameConstantUploadBuffers[MaxNumFramesInFlight];
        RenderBackendBufferHandle perFrameConstantBuffers[MaxNumFramesInFlight];

        RenderBackendBufferHandle localLightDataUploadBuffers[MaxNumFramesInFlight];
        RenderBackendBufferHandle localLightDataBuffers[MaxNumFramesInFlight];

        RenderBackendBufferHandle virtualShadowMapShaderParameterUploadBuffers[MaxNumFramesInFlight];
        RenderBackendBufferHandle virtualShadowMapShaderParameterBuffers[MaxNumFramesInFlight];

        RenderBackendBufferHandle cascadedShadowMapShaderParameterUploadBuffers[MaxNumFramesInFlight];
        RenderBackendBufferHandle cascadedShadowMapShaderParameterBuffers[MaxNumFramesInFlight];

        RenderBackendBufferHandle GetCurrentPerFrameConstantBuffer() const;

        float materialTextureMipLodBias;

        float preExposure = 1.0f;

        std::array<GeometryPassDrawCommandList, uint32(GeometryPassType::Count)> geometryPassDrawCommandLists;
        void DispatchVisibilityCulling(RenderGraph& renderGraph, const SceneView& view);
        void DispatchOpaqueGeometryPassDrawCommands(RenderBackendCommandList& commandList);
        void DispatchVirtualShadowMapPassDrawCommands(
            RenderBackendCommandList& commandList,
            const LightRenderObject& light,
            RenderBackendBufferHandle virtualShadowMapShaderParameterBuffer,
            RenderBackendBufferHandle virtualShadowMapPageTableBuffer,
            RenderBackendBufferHandle virtualShadowMapEntryBuffer,
            RenderBackendTextureHandle virtualShadowMapDepthTexture);
        void DispatchCascadedShadowMapPassDrawCommands(RenderBackendCommandList& commandList, const LightRenderObject& light, uint32 cascadeIndex, RenderBackendBufferHandle cascadeShadowMapDataBuffer);

        struct AutoExposureData
        {
            float adaptedExposure = 1.0f;
            float targetExposure = 1.0f;
            float exposureCompensation = 0.0f;
            float averageSceneLuminance = 0.0f;
        };
        static const int32 AutoExposureReadbackBufferCount = 4;
        int32 currentAutoExposureReadbackBufferIndex = 0;
        RenderGraphPersistentBuffer* autoExposureReadbackBuffers[AutoExposureReadbackBufferCount];

        AutoExposureData autoExposureData;

        void UpdateAutoExposureDataFromReadbackBuffer();

        RenderBackendBufferHandle localFogVolumeInstanceDataBufferUpload;
        uint64 localFogVolumeInstanceDataBufferSize = 0;

        // TODO
        CascadedShadowMapRenderData cascadedShadowMapRenderData;

        VirtualShadowMapManager* virtualShadowMapManager;

        std::vector<DistantLightRenderData> distanceLights;
        std::vector<LocalLightRenderData> visibleLocalLights;

        std::vector<CascadedShadowMapRenderData> cascadedShadowMaps;

        struct HistoryFrame
        {
            Vector3f cameraPosition;
            Vector2f cameraJitterOffset;
            CameraTransformations transformations;
            float preExposure;
            RenderGraphPersistentTexture* minDepthPyramidTexture = nullptr;
            RenderGraphPersistentTexture* maxDepthPyramidTexture = nullptr;
            RenderGraphPersistentBuffer* autoExposureBuffer = nullptr;
            RenderGraphPersistentTexture* exposureTexture = nullptr;
            RenderGraphPersistentTexture* sceneDepthTexture = nullptr;
            RenderGraphPersistentTexture* ambientOcclusionTexture = nullptr;
            RenderGraphPersistentTexture* screenSpaceLightShaftsTemporalFilteringTexture = nullptr;
            RenderGraphPersistentTexture* volumetricFogLightScatteringTexture = nullptr;
            RenderGraphPersistentTexture* temporalSuperSamplingOutputTexture = nullptr;
        };

        HistoryFrame historyFrame;

        RenderGraphPersistentTexture* cachedColorTransformLUTTexture = nullptr;
        PostProcessingColorTransformLUTSettings cachedColorTransformLUTSettings;

        void ResetHistoryFrame();

        // Debug draw

        RenderBackendBufferHandle debugDrawLinesVertexUploadBuffers[3];
        RenderBackendBufferHandle debugDrawLinesVertexBuffers[3];
        uint32 debugDrawLinesVertexBufferSizes[3] = { 0, 0, 0 };

        using PostProcessingDebugVisualizationCallback = std::function<RenderGraphTextureHandle(RenderGraph& renderGraph, const SceneView& view)>;
        PostProcessingDebugVisualizationCallback debugVisualizationCallback;

    public:
        std::vector<Vector3f> debugDrawLinesVertices;
        void DrawLine(
            const Vector3f& start,
            const Vector3f& end,
            const Vector4f& color,
            float width,
            uint8 depthPriorityGroup)
        {
            debugDrawLinesVertices.push_back(start);
            debugDrawLinesVertices.push_back(end);
        }

        void DrawSphere(
            const Vector3f& center,
            float radius,
            const Vector4f& color)
        {
            // x
            for (uint32 i = 0; i < 32; i++)
            {
                float theta1 = float(i) / 32.0f * 2.0f * M_PI;
                float theta2 = float(uint32((i + 1) % 32)) / 32.0f * 2.0f * M_PI;
                Vector3f c1 = Vector3f(0.0f, std::cos(theta1), std::sin(theta1));
                Vector3f c2 = Vector3f(0.0f, std::cos(theta2), std::sin(theta2));

                debugDrawLinesVertices.push_back(center + radius * c1);
                debugDrawLinesVertices.push_back(center + radius * c2);
            }

            // y
            for (uint32 i = 0; i < 32; i++)
            {
                float theta1 = float(i) / 32.0f * 2.0f * M_PI;
                float theta2 = float(uint32((i + 1) % 32)) / 32.0f * 2.0f * M_PI;
                Vector3f c1 = Vector3f(std::cos(theta1), 0.0f, std::sin(theta1));
                Vector3f c2 = Vector3f(std::cos(theta2), 0.0f, std::sin(theta2));

                debugDrawLinesVertices.push_back(center + radius * c1);
                debugDrawLinesVertices.push_back(center + radius * c2);
            }

            // z
            for (uint32 i = 0; i < 32; i++)
            {
                float theta1 = float(i) / 32.0f * 2.0f * M_PI;
                float theta2 = float(uint32((i + 1) % 32)) / 32.0f * 2.0f * M_PI;
                Vector3f c1 = Vector3f(std::cos(theta1), std::sin(theta1), 0.0f);
                Vector3f c2 = Vector3f(std::cos(theta2), std::sin(theta2), 0.0f);

                debugDrawLinesVertices.push_back(center + radius * c1);
                debugDrawLinesVertices.push_back(center + radius * c2);
            }
        }
    };
}