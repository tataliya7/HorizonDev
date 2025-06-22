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

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                    shaderConstants.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    //shaderConstants.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(depthPyramidTexture));
                    shaderConstants.BindTextureSRV(5, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(motionVectorTexture));
                    shaderConstants.BindTextureSRV(6, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(previousSceneColorTexture));
                    shaderConstants.BindTextureUAV(7, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(indirectDiffuseTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::ScreenSpaceIndirectDiffuse);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        intermediateResources.indirectDiffuseTexture = indirectDiffuseTexture;
    }
}