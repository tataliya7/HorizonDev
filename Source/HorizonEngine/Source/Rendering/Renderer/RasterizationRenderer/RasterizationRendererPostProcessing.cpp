#include "RasterizationRenderer.h"

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

        PostProcessingPipelineInputs postProcessingPipelineInputs;
        postProcessingPipelineInputs.colorTexture = colorTexture;
        postProcessingPipelineInputs.depthTexture = depthTexture;
        postProcessingPipelineInputs.motionVectorTexture = motionVectorTexture;
        postProcessingPipelineInputs.renderResolution = renderResolution;
        postProcessingPipelineInputs.targetResolution = targetResolution;
        postProcessingPipelineInputs.perFrameConstantBuffer = currentPerFrameConstantBuffer;
        postProcessingPipelineInputs.postProcessingSettings = finalPostProcessingSettings;
        postProcessingPipelineInputs.featureFlags.enableDepthOfField = renderFeatures.enableDepthOfField;
        postProcessingPipelineInputs.featureFlags.enableMotionBlur = renderFeatures.enableMotionBlur;
        postProcessingPipelineInputs.featureFlags.enableAutoExposure = renderFeatures.enableAutoExposure;
        postProcessingPipelineInputs.featureFlags.enableBilateralGridLocalToneMapping = renderFeatures.enableBilateralGridLocalToneMapping;
        postProcessingPipelineInputs.featureFlags.enableExposureFusionLocalToneMapping = renderFeatures.enableExposureFusionLocalToneMapping;
        postProcessingPipelineInputs.featureFlags.enableGaussianBloom = renderFeatures.enableGaussianBloom;
        postProcessingPipelineInputs.featureFlags.enableConvolutionBloom = renderFeatures.enableConvolutionBloom;
        postProcessingPipelineInputs.featureFlags.enableLensFlare = renderFeatures.enableLensFlare;
        postProcessingPipelineInputs.featureFlags.enableSelectionOutline = renderFeatures.enableSelectionOutline;
        postProcessingPipelineInputs.temporalSuperSamplingInterface = temporalSuperSamplingInterface;
        postProcessingPipelineInputs.viewNeedsReset = view.NeedToBeReset();

        colorTexture = postProcessingPipeline.Execute(renderGraph, view, postProcessingPipelineInputs);

        if (debugVisualizationCallback)
        {
            colorTexture = debugVisualizationCallback(renderGraph, view);
        }

        intermediateResources.hudLessColorTexture = colorTexture;
    }
}