#include "PostProcessing.h"
#include "../RasterizationRenderer.h"
#include "../TemporalSuperSampling.h"

namespace Horizon
{
    void RasterizationRenderer::ExecutePostProcessingPipeline(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        OPTICK_EVENT();
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "PostProcessingPipeline");

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        RenderGraphTextureHandle colorTexture = intermediateResources.colorTexture;
        RenderGraphTextureHandle depthTexture = intermediateResources.depthTexture;
        RenderGraphTextureHandle motionVectorTexture = intermediateResources.motionVectorTexture;
        RenderGraphTextureHandle blackDummyTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        RenderGraphTextureHandle whiteDummyTexture = defaultResources->ImportWhiteDummyTexture2D(renderGraph);

        RenderGraphTextureHandle exposureTexture = renderGraph.ImportExternalTexture(historyFrame.exposureTexture, "PreviousExposureTexture");
        if (!exposureTexture)
        {
            exposureTexture = whiteDummyTexture;
        }
        else if (renderBackend->GetType() == RenderBackendType::D3D12)
        {
            // @todo Fix me!
            renderGraph.AddPass(
                std::format("FixMe! (Compute, {}x{})", 1, 1),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendBarrier transitions[1] =
                        {
                            RenderBackendBarrier(
                                resourceRegistry.GetRenderBackendTextureHandle(exposureTexture),
                                RenderBackendTextureSubresourceRange(0, 1, 0, 1),
                                RenderBackendResourceState::Undefined,
                                RenderBackendResourceState::ShaderResource)
                        };
                        commandList.Barriers(transitions, 1);
                    };
                });
        }

        RenderGraphBufferHandle previousAutoExposureBuffer = renderGraph.ImportExternalBuffer(historyFrame.autoExposureBuffer, "PreviousAutoExposureBuffer");
        RenderGraphBufferHandle autoExposureBuffer = previousAutoExposureBuffer;

        if (renderFeatures.enableDepthOfField)
        {
            colorTexture = DispatchDepthOfField(renderGraph, view, colorTexture);
        }

        if (renderFeatures.enableTemporalSuperSampling)
        {
            TemporalSuperSamplingDispatchDescription temporalSuperSamplingDispatchDescription;
            temporalSuperSamplingDispatchDescription.colorTexture = colorTexture;
            temporalSuperSamplingDispatchDescription.depthTexture = depthTexture;
            temporalSuperSamplingDispatchDescription.motionVectorTexture = motionVectorTexture;
            temporalSuperSamplingDispatchDescription.exposureTexture = exposureTexture;
            colorTexture = DispatchTemporalSuperSampling(temporalSuperSamplingInterface, renderGraph, view, temporalSuperSamplingDispatchDescription);

            renderGraph.ExportTextureDeferred(colorTexture, &historyFrame.temporalSuperSamplingOutputTexture);
        }

        if (renderFeatures.enableMotionBlur)
        {
            colorTexture = DispatchMotionBlur(renderGraph, view, colorTexture, depthTexture, motionVectorTexture);
        }

        // @todo Determine when to build a color pyramid and how many levels to build.
        const bool shouldBuildColorPyramid = true;

        PostProcessingColorPyramid colorPyramid;
        if (shouldBuildColorPyramid)
        {
            DispatchColorPyramidGeneration(renderGraph, view, colorTexture, &colorPyramid);
        }

        // The auto exposure pass is always executed.
        // When fixed exposure is enabled, the auto exposure pass will force output the specified exposure value.
        {
            autoExposureBuffer = DispatchHistogramBasedAutoExposure(renderGraph, view, colorTexture, colorPyramid, previousAutoExposureBuffer);

            // Copy the final exposure value to a 1x1 texture.
            exposureTexture = AddCopyExposurePass(renderGraph, view, autoExposureBuffer);

            renderGraph.ExportTextureDeferred(exposureTexture, &historyFrame.exposureTexture);
        }

        RenderGraphTextureHandle localToneMappingTexture = whiteDummyTexture;
        if (renderFeatures.enableBilateralGridLocalToneMapping)
        {
            localToneMappingTexture = DispatchBilateralGridLocalToneMapping(renderGraph, view, colorTexture, colorPyramid, autoExposureBuffer);
        }
        else if (renderFeatures.enableExposureFusionLocalToneMapping)
        {
            localToneMappingTexture = DispatchExposureFusionLocalToneMapping(renderGraph, view, colorTexture, colorPyramid, exposureTexture);
        }

        RenderGraphTextureHandle bloomTexture = blackDummyTexture;

        const bool isBloomEnabled = renderFeatures.enableGaussianBloom || renderFeatures.enableConvolutionBloom;
        if (isBloomEnabled)
        {
            if (renderFeatures.enableGaussianBloom)
            {
                bloomTexture = DispatchGaussianBloom(renderGraph, view, colorTexture, colorPyramid);
            }
            else if (renderFeatures.enableConvolutionBloom)
            {
                bloomTexture = DispatchConvolutionBloom(renderGraph, view, colorTexture, colorPyramid);
            }

            if (renderFeatures.enableLensFlare)
            {
                bloomTexture = DispatchLensFlarePass(renderGraph, view, colorTexture, colorPyramid, bloomTexture);
            }
        }

        RenderGraphTextureHandle colorTransformLUTTexture = RenderColorTransformLUT(renderGraph, view);

        // @todo Add HDR support.
        colorTexture = DispatchFinalComposition(
            renderGraph,
            view,
            colorTexture,
            bloomTexture,
            localToneMappingTexture,
            colorTransformLUTTexture,
            autoExposureBuffer);

        RenderGraphTextureHandle colorTextureAfterToneMapping = colorTexture;

#if HORIZON_EDITOR
        if (renderFeatures.enableEditorSelectionOutline)
        {
            colorTexture = DispatchEditorSelectionOutline(renderGraph, view, colorTexture);
        }
#endif

        if (debugVisualizationCallback)
        {
            colorTexture = debugVisualizationCallback(renderGraph, view);
        }

        intermediateResources.hudLessColorTexture = colorTexture;
    }
}