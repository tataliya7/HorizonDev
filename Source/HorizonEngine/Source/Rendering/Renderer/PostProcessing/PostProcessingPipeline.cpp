#include "PostProcessingPipeline.h"

namespace Horizon
{
    PostProcessingPipeline::PostProcessingPipeline(
        RenderBackend* renderBackend,
        RenderGraphResourcePool* resourcePool,
        ShaderRepository* shaderRepository,
        RendererDefaultResources* defaultResources)
        : renderBackend(renderBackend)
        , resourcePool(resourcePool)
        , shaderRepository(shaderRepository)
        , defaultResources(defaultResources)
    {
        AutoExposureData defaultAutoExposureData;
        RenderBackendBufferDescription autoExposureReadbackBufferDesc = RenderBackendBufferDescription::CreateReadback(sizeof(AutoExposureData));
        for (uint32 index = 0; index < AutoExposureReadbackBufferCount; index++)
        {
            RenderBackendBufferHandle autoExposureReadbackBuffer = renderBackend->CreateBuffer(&autoExposureReadbackBufferDesc, &defaultAutoExposureData, "AutoExposureReadBackBuffer");
            autoExposureReadbackBuffers[index] = resourcePool->CacheBuffer(autoExposureReadbackBuffer, autoExposureReadbackBufferDesc, "AutoExposureReadBackBuffer");
        }

        ResetHistoricalData();
    }

    PostProcessingPipeline::~PostProcessingPipeline()
    {
    }

    void PostProcessingPipeline::ResetHistoricalData()
    {
        AutoExposureData defaultAutoExposureData;
        RenderBackendBufferDescription autoExposureBufferDesc = RenderBackendBufferDescription::Create(sizeof(AutoExposureData), 1, RenderBackendBufferCreateFlags::ShaderResource | RenderBackendBufferCreateFlags::UnorderedAccess);
        RenderBackendBufferHandle autoExposureBuffer = renderBackend->CreateBuffer(&autoExposureBufferDesc, &defaultAutoExposureData, "AutoExposureBuffer");
        historicalData.autoExposureBuffer = resourcePool->CacheBuffer(autoExposureBuffer, autoExposureBufferDesc, "AutoExposureBuffer");
        historicalData.exposureTexture = nullptr;
    }

    RenderBackendBufferHandle PostProcessingPipeline::GetCurrentPerFrameConstantBuffer() const
    {
        return currentPerFrameConstantBuffer;
    }

    RenderGraphTextureHandle PostProcessingPipeline::Execute(
        RenderGraph& renderGraph,
        const SceneView& view,
        const PostProcessingPipelineInputs& inputs)
    {
        OPTICK_EVENT();
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "PostProcessingPipeline");

        renderResolution = inputs.renderResolution;
        targetResolution = inputs.targetResolution;
        currentPerFrameConstantBuffer = inputs.perFrameConstantBuffer;
        postProcessingSettings = inputs.postProcessingSettings;

        RenderGraphTextureHandle colorTexture = inputs.colorTexture;
        RenderGraphTextureHandle depthTexture = inputs.depthTexture;
        RenderGraphTextureHandle motionVectorTexture = inputs.motionVectorTexture;
        RenderGraphTextureHandle blackDummyTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        RenderGraphTextureHandle whiteDummyTexture = defaultResources->ImportWhiteDummyTexture2D(renderGraph);

        RenderGraphTextureHandle exposureTexture = renderGraph.ImportExternalTexture(historicalData.exposureTexture, "PreviousExposureTexture");
        if (!exposureTexture)
        {
            exposureTexture = whiteDummyTexture;
        }

        RenderGraphBufferHandle previousAutoExposureBuffer = renderGraph.ImportExternalBuffer(historicalData.autoExposureBuffer, "PreviousAutoExposureBuffer");
        RenderGraphBufferHandle autoExposureBuffer = previousAutoExposureBuffer;

        if (inputs.featureFlags.enableDepthOfField)
        {
            colorTexture = DispatchDepthOfField(renderGraph, view, colorTexture);
        }

        if (inputs.temporalSuperSamplingInterface)
        {
            TemporalSuperSamplingDispatchDescription temporalSuperSamplingDispatchDescription;
            temporalSuperSamplingDispatchDescription.colorTexture = colorTexture;
            temporalSuperSamplingDispatchDescription.depthTexture = depthTexture;
            temporalSuperSamplingDispatchDescription.motionVectorTexture = motionVectorTexture;
            temporalSuperSamplingDispatchDescription.exposureTexture = exposureTexture;
            colorTexture = DispatchTemporalSuperSampling(inputs.temporalSuperSamplingInterface, renderGraph, view, temporalSuperSamplingDispatchDescription);

            renderGraph.ExportTextureDeferred(colorTexture, &historicalData.temporalSuperSamplingOutputTexture);
        }

        if (inputs.featureFlags.enableMotionBlur)
        {
            colorTexture = DispatchMotionBlur(renderGraph, view, colorTexture, depthTexture, motionVectorTexture);
        }

        const bool shouldBuildColorPyramid = true;

        PostProcessingColorPyramid colorPyramid;
        if (shouldBuildColorPyramid)
        {
            DispatchColorPyramidGeneration(renderGraph, view, colorTexture, &colorPyramid);
        }

        {
            autoExposureBuffer = DispatchHistogramBasedAutoExposure(renderGraph, view, colorTexture, colorPyramid, previousAutoExposureBuffer);

            exposureTexture = AddCopyExposurePass(renderGraph, view, autoExposureBuffer);

            renderGraph.ExportTextureDeferred(exposureTexture, RenderBackendResourceState::ShaderResource, &historicalData.exposureTexture);
        }

        RenderGraphTextureHandle localToneMappingTexture = whiteDummyTexture;
        if (inputs.featureFlags.enableBilateralGridLocalToneMapping)
        {
            localToneMappingTexture = DispatchBilateralGridLocalToneMapping(renderGraph, view, colorTexture, colorPyramid, autoExposureBuffer);
        }
        else if (inputs.featureFlags.enableExposureFusionLocalToneMapping)
        {
            localToneMappingTexture = DispatchExposureFusionLocalToneMapping(renderGraph, view, colorTexture, colorPyramid, exposureTexture);
        }

        RenderGraphTextureHandle bloomTexture = blackDummyTexture;

        const bool isBloomEnabled = inputs.featureFlags.enableGaussianBloom || inputs.featureFlags.enableConvolutionBloom;
        if (isBloomEnabled)
        {
            if (inputs.featureFlags.enableGaussianBloom)
            {
                bloomTexture = DispatchGaussianBloom(renderGraph, view, colorTexture, colorPyramid);
            }
            else if (inputs.featureFlags.enableConvolutionBloom)
            {
                bloomTexture = DispatchConvolutionBloom(renderGraph, view, colorTexture, colorPyramid);
            }

            if (inputs.featureFlags.enableLensFlare)
            {
                bloomTexture = DispatchLensFlare(renderGraph, view, colorTexture, colorPyramid, bloomTexture);
            }
        }

        RenderGraphTextureHandle colorTransformLUTTexture = ComputeColorTransformLUT(renderGraph, view);

        colorTexture = DispatchFinalComposition(
            renderGraph,
            view,
            colorTexture,
            bloomTexture,
            localToneMappingTexture,
            colorTransformLUTTexture,
            autoExposureBuffer);

        if (inputs.featureFlags.enableSelectionOutline)
        {
            colorTexture = DispatchSelectionOutline(renderGraph, view, colorTexture);
        }

        return colorTexture;
    }
}
