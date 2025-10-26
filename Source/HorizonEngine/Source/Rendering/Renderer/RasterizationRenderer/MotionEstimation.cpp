#include "RasterizationRenderer.h"

namespace Horizon
{
    void RasterizationRenderer::DispatchMotionVectorEstimation(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        GPUSceneRenderGraphResources& gpuSceneResources = renderGraph.blackboard.Get<GPUSceneRenderGraphResources>();
        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        const uint32 motionVectorTextureWidth = renderResolution.width;
        const uint32 motionVectorTextureHeight = renderResolution.height;

        RenderGraphTextureDescription motionVectorTextureDescription = RenderGraphTextureDescription::Create2D(
            motionVectorTextureWidth,
            motionVectorTextureHeight,
            RenderBackendTextureFormat::R16G16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        intermediateResources.motionVectorTexture = renderGraph.CreateTexture(motionVectorTextureDescription, "MotionVectorTexture");

        renderGraph.AddPass(
            std::format("MotionVectorEstimation (Compute, {}x{})", motionVectorTextureWidth, motionVectorTextureHeight),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, gpuSceneResources.geometryDataBuffer);
                builder.SetBindlessResourceSRV(2, gpuSceneResources.geometryInstanceDataBuffer);
                builder.SetBindlessResourceSRV(3, intermediateResources.visibleMeshletBuffer);
                builder.SetBindlessResourceSRV(4, intermediateResources.depthTexture);
                builder.SetBindlessResourceSRV(5, intermediateResources.vbuffer0);
                builder.SetBindlessResourceSRV(6, intermediateResources.vbuffer1);
                builder.SetBindlessResourceUAV(7, intermediateResources.motionVectorTexture, 0);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::MotionVectorEstimation);

                uint32 threadGroupCountX = ComputeShaderThreadGroupCount(motionVectorTextureWidth, 8);
                uint32 threadGroupCountY = ComputeShaderThreadGroupCount(motionVectorTextureHeight, 8);
                uint32 threadGroupCountZ = 1;

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });
    }
}