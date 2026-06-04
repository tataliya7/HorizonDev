#pragma once

#include "PostProcessingSettings.h"
#include "Rendering/RenderBackend/RenderBackendModule.h"
#include "Rendering/RenderGraph/RenderGraph.h"
#include "Rendering/Renderer/ShaderRepository.h"
#include "Rendering/Renderer/SceneView.h"
#include "Rendering/Renderer/RenderUtility.h"
#include "Rendering/Renderer/TemporalSuperSampling.h"

namespace Horizon
{
    constexpr uint32 PostProcessingThreadGroupSizeX = 8;
    constexpr uint32 PostProcessingThreadGroupSizeY = 8;

    struct PostProcessingColorPyramid
    {
        static constexpr uint32 MaxMipLevelCount = 5;

        uint32 mipLevelCount = 0;

        RenderGraphTextureHandle textures[MaxMipLevelCount] = {};
    };

    struct PostProcessingColorTransformLUTSettings
    {
        bool initialized = false;
        float whiteBalance;

        bool Update(const SceneView& view, const PostProcessingSettings& postProcessingSettings);
    };

    struct PostProcessingFeatureFlags
    {
        bool enableDepthOfField = false;
        bool enableMotionBlur = false;
        bool enableAutoExposure = false;
        bool enableBilateralGridLocalToneMapping = false;
        bool enableExposureFusionLocalToneMapping = false;
        bool enableGaussianBloom = false;
        bool enableConvolutionBloom = false;
        bool enableLensFlare = false;
        bool enableSelectionOutline = false;
    };

    struct PostProcessingPipelineInputs
    {
        RenderGraphTextureHandle colorTexture;
        RenderGraphTextureHandle depthTexture;
        RenderGraphTextureHandle motionVectorTexture;
        Extent2D renderResolution;
        Extent2D targetResolution;
        RenderBackendBufferHandle perFrameConstantBuffer;
        PostProcessingSettings postProcessingSettings;
        PostProcessingFeatureFlags featureFlags;
        bool viewNeedsReset = false;
        TemporalSuperSamplingInterface* temporalSuperSamplingInterface = nullptr;
    };

    struct PostProcessingHistoricalData
    {
        RenderGraphPersistentBuffer* autoExposureBuffer = nullptr;
        RenderGraphPersistentTexture* exposureTexture = nullptr;
        RenderGraphPersistentTexture* temporalSuperSamplingOutputTexture = nullptr;
    };

    class PostProcessingPipeline
    {
    public:

        PostProcessingPipeline(
            RenderBackend* renderBackend,
            RenderGraphResourcePool* resourcePool,
            ShaderRepository* shaderRepository,
            RendererDefaultResources* defaultResources);

        ~PostProcessingPipeline();

        RenderGraphTextureHandle Execute(
            RenderGraph& renderGraph,
            const SceneView& view,
            const PostProcessingPipelineInputs& inputs);

        void ResetHistoricalData();

        struct AutoExposureData
        {
            float adaptedExposure = 1.0f;
            float targetExposure = 1.0f;
            float exposureCompensation = 0.0f;
            float averageSceneLuminance = 0.0f;
        };

        const AutoExposureData& GetAutoExposureData() const { return autoExposureData; }

        void UpdateAutoExposureDataFromReadbackBuffer();

    private:

        RenderGraphTextureHandle DispatchDepthOfField(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle sceneColor);

        RenderGraphTextureHandle DispatchMotionBlur(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle colorTexture,
            RenderGraphTextureHandle depthTexture,
            RenderGraphTextureHandle velocityTexture);

        void DispatchColorPyramidGeneration(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle colorTexture,
            PostProcessingColorPyramid* outMipChain);

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

        RenderGraphTextureHandle DispatchLensFlare(
            RenderGraph& renderGraph,
            const SceneView& view,
            RenderGraphTextureHandle colorTexture,
            const PostProcessingColorPyramid& colorPyramid,
            RenderGraphTextureHandle bloomTexture);

        RenderGraphTextureHandle ComputeColorTransformLUT(
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

        RenderGraphTextureHandle DispatchSelectionOutline(
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

        RenderBackend* renderBackend;
        RenderGraphResourcePool* resourcePool;
        ShaderRepository* shaderRepository;
        RendererDefaultResources* defaultResources;

        Extent2D renderResolution;
        Extent2D targetResolution;
        RenderBackendBufferHandle currentPerFrameConstantBuffer;
        PostProcessingSettings postProcessingSettings;

        AutoExposureData autoExposureData;
        static constexpr int32 AutoExposureReadbackBufferCount = 4;
        int32 currentAutoExposureReadbackBufferIndex = 0;
        RenderGraphPersistentBuffer* autoExposureReadbackBuffers[AutoExposureReadbackBufferCount];

        PostProcessingHistoricalData historicalData;

        RenderGraphPersistentTexture* cachedColorTransformLUTTexture = nullptr;
        PostProcessingColorTransformLUTSettings cachedColorTransformLUTSettings;

        RenderBackendBufferHandle GetCurrentPerFrameConstantBuffer() const;
    };
}
