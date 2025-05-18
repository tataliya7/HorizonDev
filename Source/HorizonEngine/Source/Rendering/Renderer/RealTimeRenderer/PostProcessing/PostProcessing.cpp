#include "PostProcessing.h"
#include "../RealTimeRenderer.h"
#include "../TemporalSuperSampling.h"

namespace Horizon
{
    void RealTimeRenderer::ExecutePostProcessingPipeline(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        RenderGraphTextureHandle colorTexture = sceneTextures.sceneColorTexture;
        RenderGraphTextureHandle depthTexture = sceneTextures.sceneDepthTexture;
        RenderGraphTextureHandle motionVectorTexture = sceneTextures.motionVectorTexture;

        RenderGraphBufferHandle previousAutoExposureBuffer = renderGraph.ImportExternalBuffer(historyFrame.autoExposureBuffer, "PreviousAutoExposureBuffer");
        RenderGraphBufferHandle autoExposureBuffer = previousAutoExposureBuffer;

        const bool shouldBuildColorPyramid = true;// IsBloomEnabled();

#if HORIZON_EDITOR
        const bool isEditorSelectionOutlineEnabled = true;
        const bool isEditorGizmosEnabled = true;
#endif

        const bool isVisualizeDepthEnabled                = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::Depth);
        const bool isVisualizePrimitiveIDEnabled          = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::PrimitiveID);
        const bool isVisualizeMaterialIDEnabled           = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::MaterialID);
        const bool isVisualizeWorldSpaceNormalEnabled     = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::WorldSpaceNormal);
        const bool isVisualizeMotionVectorsEnabled        = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::MotionVectors);
        const bool isVisualizeAmbientOcclusionEnabled     = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::AmbientOcclusion);
        const bool isVisualizeShadowMaskEnabled           = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::ShadowMask);
        const bool isVisualizeCascadedShadowMapEnabled    = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::CascadedShadowMapCascadeIndex);

        if (IsDepthOfFieldEnabled())
        {
            colorTexture = DispatchDepthOfField(renderGraph, view, colorTexture);
        }

        colorTexture = AddDebugDrawPass(renderGraph, view, colorTexture, depthTexture);

        if (IsSuperResolutionEnabled()) // TODO
        {
            RenderGraphTextureHandle exposureTexture = AddCopyExposurePass(renderGraph, view, previousAutoExposureBuffer);

            TemporalSuperSamplingDispatchDescription tssDispatchDescription;
            tssDispatchDescription.colorTexture = colorTexture;
            tssDispatchDescription.depthTexture = depthTexture;
            tssDispatchDescription.motionVectorTexture = motionVectorTexture;
            tssDispatchDescription.exposureTexture = exposureTexture;
            colorTexture = DispatchCustomTemporalSuperSampling(temporalSuperSamplingInterface, renderGraph, view, tssDispatchDescription);

            renderGraph.ExportTextureDeferred(colorTexture, &historyFrame.temporalSuperSamplingOutputTexture);
        }

        if (IsMotionBlurEnabled())
        {
            colorTexture = DispatchMotionBlur(renderGraph, view, colorTexture, depthTexture, motionVectorTexture);
        }

        //sceneColorTexture = renderGraph.ImportExternalTexture(localToneMappingTestTexture, localToneMappingTestTextureDesc, RenderBackendResourceState::ShaderResource, "Test");
        PostProcessingColorPyramid colorPyramid;
        if (shouldBuildColorPyramid)
        {
            DispatchColorPyramidGeneration(renderGraph, view, colorTexture, &colorPyramid);
        }

        // The auto exposure pass is always executed.
        // When fixed exposure is enabled, the auto exposure pass will force output the specified exposure.
        {
            autoExposureBuffer = DispatchHistogramBasedAutoExposure(renderGraph, view, colorPyramid.textures[0], previousAutoExposureBuffer);
        }
        RenderGraphTextureHandle exposureTexture = AddCopyExposurePass(renderGraph, view, autoExposureBuffer);

        RenderGraphTextureHandle localToneMappingTexture = defaultResources->ImportWhiteDummyTexture2D(renderGraph);
        if (IsLocalToneMappingEnabled())
        {
            if (finalPostProcessingSettings.localToneMappingMethod == LocalToneMappingMethod::BilateralGrid)
            {
                localToneMappingTexture = DispatchBilateralGridToneMapping(renderGraph, view, colorPyramid, colorTexture, autoExposureBuffer);
            }
            else if (finalPostProcessingSettings.localToneMappingMethod == LocalToneMappingMethod::ExposureFusion)
            {
                localToneMappingTexture = DispatchExposureFusionLocalToneMapping(renderGraph, view, colorTexture, exposureTexture);
            }
        }

        RenderGraphTextureHandle bloomTexture = RenderGraphTextureHandle::Null;
        if (IsBloomEnabled())
        {
            if (IsGaussianBloomEnabled())
            {
                bloomTexture = DispatchGaussianBloom(renderGraph, view, colorPyramid.textures[0]);
            }
            else
            {
                assert(IsConvolutionBloomEnabled());
                bloomTexture = DispatchConvolutionBloom(renderGraph, view, colorPyramid.textures[0]);
            }

            if (IsLensFlareEnabled())
            {
                bloomTexture = AddLensFlarePass(renderGraph, view, colorPyramid.textures[1], bloomTexture);
            }
        }

        RenderGraphTextureHandle colorLUTTexture = RenderColorTransformLUT(renderGraph, view);

        bool outputInHDR = false;

        colorTexture = DispatchFinalComposition(
            renderGraph,
            view,
            colorTexture,
            bloomTexture,
            localToneMappingTexture,
            colorLUTTexture,
            autoExposureBuffer,
            outputInHDR);

        RenderGraphTextureHandle colorTextureAfterToneMapping = colorTexture;

#if WITH_HORIZON_EDITOR
        // if (isEditorSelectionOutlineEnabled)
        // {
        //     sceneColorTexture = AddEditorSelectionOutlinePass(renderGraph, view, sceneColorTexture);
        // }
        //
        // if (isEditorGizmosEnabled)
        // {
        //     sceneColorTexture = AddEditorGizmosPass(renderGraph, view, sceneColorTexture);
        // }
#endif

        if (isVisualizeDepthEnabled)
        {
            colorTexture = AddVisualizeDepthPass(renderGraph, view);
        }
        if (isVisualizePrimitiveIDEnabled)
        {
            colorTexture = AddVisualizePrimitiveIDPass(renderGraph, view);
        }
        if (isVisualizeMaterialIDEnabled)
        {
            colorTexture = AddVisualizeMaterialIDPass(renderGraph, view);
        }
        if (isVisualizeWorldSpaceNormalEnabled)
        {
            colorTexture = AddVisualizeWorldSpaceNormalPass(renderGraph, view);
        }
        if (isVisualizeMotionVectorsEnabled)
        {
            colorTexture = AddVisualizeMotionVectorsPass(renderGraph, view);
        }
        if (isVisualizeAmbientOcclusionEnabled)
        {
            colorTexture = AddVisualizeAmbientOcclusionPass(renderGraph, view);
        }
        if (isVisualizeShadowMaskEnabled)
        {
            colorTexture = AddVisualizeShadowMaskPass(renderGraph, view, colorTexture);
        }
        if (isVisualizeCascadedShadowMapEnabled)
        {
            colorTexture = AddVisualizeCascadedShadowMapPass(renderGraph, view);
        }

        sceneTextures.hudLessColorTexture = colorTexture;
    }
}