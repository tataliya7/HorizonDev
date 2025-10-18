#include "RasterizationRenderer.h"

namespace Horizon
{
    void RasterizationRenderer::RenderScreenSpaceIndirectDiffuse(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RenderGraphTextureHandle previousSceneColorTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        if (historyFrame.temporalSuperSamplingOutputTexture)
        {
            previousSceneColorTexture = renderGraph.ImportExternalTexture(historyFrame.temporalSuperSamplingOutputTexture, "SSGIInputColorTexture");
        }

        RenderGraphTextureDescription indirectDiffuseTextureDesc = RenderGraphTextureDescription::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        RenderGraphTextureHandle indirectDiffuseTexture = renderGraph.CreateTexture(indirectDiffuseTextureDesc, "ScreenSpaceIndirectDiffuseTexture");

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        renderGraph.AddPass(
            std::format("ScreenSpaceIndirectDiffuse (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle gbuffer0 = builder.ReadTexture(intermediateResources.gbuffer0, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(intermediateResources.gbuffer1, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::ShaderResource);
                //RenderGraphTextureHandle depthPyramidTexture = builder.ReadTexture(intermediateResources.depthPyramidTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle motionVectorTexture = builder.ReadTexture(intermediateResources.motionVectorTexture, RenderBackendResourceState::ShaderResource);
                previousSceneColorTexture = builder.ReadTexture(previousSceneColorTexture, RenderBackendResourceState::ShaderResource);
                indirectDiffuseTexture = builder.WriteTexture(indirectDiffuseTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(indirectDiffuseTextureDesc.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(indirectDiffuseTextureDesc.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                    pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    //pushConstantValues.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(depthPyramidTexture));
                    pushConstantValues.BindTextureSRV(5, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(motionVectorTexture));
                    pushConstantValues.BindTextureSRV(6, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(previousSceneColorTexture));
                    pushConstantValues.BindTextureUAV(7, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(indirectDiffuseTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::ScreenSpaceIndirectDiffuse);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        intermediateResources.indirectDiffuseTexture = indirectDiffuseTexture;
    }
}