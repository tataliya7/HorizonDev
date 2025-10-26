#include "RasterizationRenderer.h"

namespace Horizon
{
    void RasterizationRenderer::DispatchMotionVectorEstimation(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const GPUScene* gpuScene = view.scene->GetGPUScene();
        RenderGraphBufferHandle geometryDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryDataBuffer);
        RenderGraphBufferHandle geometryInstanceDataBuffer = renderGraph.ImportExternalBuffer(gpuScene->persistentGeometryInstanceDataBuffer);

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        renderGraph.AddPass(
            std::format("MotionVectorEstimation (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, geometryDataBuffer);
                builder.SetBindlessResourceSRV(2, geometryInstanceDataBuffer);
                builder.SetBindlessResourceSRV(3, intermediateResources.visibleMeshletBuffer);
                builder.SetBindlessResourceSRV(4, intermediateResources.depthTexture);
                builder.SetBindlessResourceSRV(5, intermediateResources.vbuffer0);
                builder.SetBindlessResourceSRV(6, intermediateResources.vbuffer1);
                builder.SetBindlessResourceUAV(7, intermediateResources.motionVectorTexture, 0);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::MotionVectorEstimation);

                uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, 8);
                uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, 8);
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