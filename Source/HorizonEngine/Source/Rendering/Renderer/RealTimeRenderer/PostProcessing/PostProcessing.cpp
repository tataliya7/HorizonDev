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

        RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture;
        RenderGraphTextureHandle sceneDepthTexture = sceneTextures.sceneDepthTexture;
        RenderGraphTextureHandle motionVectorTexture = sceneTextures.motionVectorTexture;

        RenderGraphBufferHandle previousAutoExposureBuffer = renderGraph.ImportExternalBuffer(historyFrame.autoExposureBuffer, "PreviousAutoExposureBuffer");
        RenderGraphBufferHandle autoExposureBuffer = previousAutoExposureBuffer;

        const bool shouldRenderSceneColorPyramid = true;// IsBloomEnabled();

#if HORIZON_EDITOR
        const bool isEditorSelectionOutlineEnabled = true;
        const bool isEditorGizmosEnabled = true;
#endif

        const bool isVisualizeDepthEnabled               = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::Depth);
        const bool isVisualizePrimitiveIDEnabled         = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::PrimitiveID);
        const bool isVisualizeMaterialIDEnabled          = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::MaterialID);
        const bool isVisualizeWorldSpaceNormalEnabled    = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::WorldSpaceNormal);
        const bool isVisualizeMotionVectorsEnabled       = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::MotionVectors);
        const bool isVisualizeAmbientOcclusionEnabled    = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::AmbientOcclusion);
        const bool isVisualizeShadowMaskEnabled          = (view.debugVisualizationMode == SceneViewDebugVisualizationMode::ShadowMask);

        if (IsDepthOfFieldEnabled())
        {
            sceneColorTexture = DispatchDepthOfField(renderGraph, view, sceneColorTexture);
        }

        sceneColorTexture = AddDebugDrawPass(renderGraph, view, sceneColorTexture, sceneDepthTexture);

        if (IsSuperResolutionEnabled()) // TODO
        {
            RenderGraphTextureHandle exposureTexture = AddCopyExposurePass(renderGraph, view, previousAutoExposureBuffer);

            TemporalSuperSamplingDispatchDescription tssDispatchDescription;
            tssDispatchDescription.colorTexture = sceneColorTexture;
            tssDispatchDescription.depthTexture = sceneDepthTexture;
            tssDispatchDescription.motionVectorTexture = motionVectorTexture;
            tssDispatchDescription.exposureTexture = exposureTexture;
            sceneColorTexture = DispatchCustomTemporalSuperSampling(temporalSuperSamplingInterface, renderGraph, view, tssDispatchDescription);

            renderGraph.ExportTextureDeferred(sceneColorTexture, &historyFrame.temporalSuperSamplingOutputTexture);
        }

        if (IsMotionBlurEnabled())
        {
            sceneColorTexture = DispatchMotionBlur(renderGraph, view, sceneColorTexture, sceneDepthTexture, motionVectorTexture);
        }

        //sceneColorTexture = renderGraph.ImportExternalTexture(localToneMappingTestTexture, localToneMappingTestTextureDesc, RenderBackendResourceState::ShaderResource, "Test");
        PostProcessingSceneColorMipChain sceneColorMipChain;
        if (shouldRenderSceneColorPyramid)
        {
            RenderSceneColorPyramid(renderGraph, view, sceneColorTexture, &sceneColorMipChain);
        }

        // The auto exposure pass is always executed.
        // When fixed exposure is enabled, the auto exposure pass will force output the specified exposure.
        {
            autoExposureBuffer = DispatchHistogramBasedAutoExposure(renderGraph, view, sceneColorMipChain.textures[0], previousAutoExposureBuffer);
        }
        RenderGraphTextureHandle exposureTexture = AddCopyExposurePass(renderGraph, view, autoExposureBuffer);

        RenderGraphTextureHandle exposureFusionOuptutTexture = RenderGraphTextureHandle::Null;
        if (IsLocalToneMappingEnabled())
        {
            exposureFusionOuptutTexture = DispatchExposureFusion(renderGraph, view, sceneColorTexture, exposureTexture);
        }

        RenderGraphTextureHandle bloomTexture = RenderGraphTextureHandle::Null;
        if (IsBloomEnabled())
        {
            if (IsGaussianBloomEnabled())
            {
                bloomTexture = DispatchGaussianBloom(renderGraph, view, sceneColorMipChain.textures[0]);
            }
            else
            {
                assert(IsConvolutionBloomEnabled());
                bloomTexture = DispatchConvolutionBloom(renderGraph, view, sceneColorMipChain.textures[0]);
            }

            if (IsLensFlareEnabled())
            {
                bloomTexture = AddLensFlarePass(renderGraph, view, sceneColorMipChain.textures[1], bloomTexture);
            }
        }

        RenderGraphTextureHandle colorLUTTexture = RenderColorTransformLUT(renderGraph, view);

        bool outputInHDR = false;

        sceneColorTexture = DispatchFinalComposition(renderGraph, view, sceneColorTexture, bloomTexture, exposureFusionOuptutTexture, colorLUTTexture, autoExposureBuffer, outputInHDR);

        RenderGraphTextureHandle sceneColorTextureAfterToneMapping = sceneColorTexture;

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
            sceneColorTexture = AddVisualizeDepthPass(renderGraph, view);
        }
        if (isVisualizePrimitiveIDEnabled)
        {
            sceneColorTexture = AddVisualizePrimitiveIDPass(renderGraph, view);
        }
        if (isVisualizeMaterialIDEnabled)
        {
            sceneColorTexture = AddVisualizeMaterialIDPass(renderGraph, view);
        }
        if (isVisualizeWorldSpaceNormalEnabled)
        {
            sceneColorTexture = AddVisualizeWorldSpaceNormalPass(renderGraph, view);
        }
        if (isVisualizeMotionVectorsEnabled)
        {
            sceneColorTexture = AddVisualizeMotionVectorsPass(renderGraph, view);
        }
        if (isVisualizeAmbientOcclusionEnabled)
        {
            sceneColorTexture = AddVisualizeAmbientOcclusionPass(renderGraph, view);
        }
        if (isVisualizeShadowMaskEnabled)
        {
            sceneColorTexture = AddVisualizeShadowMaskPass(renderGraph, view, sceneColorTexture);
        }

        sceneTextures.hudLessColorTexture = sceneColorTexture;
    }
}