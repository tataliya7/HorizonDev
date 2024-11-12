#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::RenderScreenSpaceIndirectDiffuse(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RenderGraphTextureHandle previousSceneColorTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        if (historyFrame.temporalSuperSamplingOutputTexture)
        {
            previousSceneColorTexture = renderGraph.ImportExternalTexture(historyFrame.temporalSuperSamplingOutputTexture, "SSGIInputColorTexture");
        }

        RenderGraphTextureDesc indirectDiffuseTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        RenderGraphTextureHandle indirectDiffuseTexture = renderGraph.CreateTexture(indirectDiffuseTextureDesc, "ScreenSpaceIndirectDiffuseTexture");

        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        renderGraph.AddPass(
            std::format("ScreenSpaceIndirectDiffuse (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                //RenderGraphTextureHandle depthPyramidTexture = builder.ReadTexture(sceneTextures.depthPyramidTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle motionVectorTexture = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);
                previousSceneColorTexture = builder.ReadTexture(previousSceneColorTexture, RenderBackendResourceState::ShaderResource);
                indirectDiffuseTexture = builder.WriteTexture(indirectDiffuseTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = CeilDiv(indirectDiffuseTextureDesc.width, 8);
                    uint32 threadGroupCountY = CeilDiv(indirectDiffuseTextureDesc.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    //shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(depthPyramidTexture));
                    shaderConstants.BindTextureSRV(5, registry.GetTextureSRVBindlessResourceDescriptorIndex(motionVectorTexture));
                    shaderConstants.BindTextureSRV(6, registry.GetTextureSRVBindlessResourceDescriptorIndex(previousSceneColorTexture));
                    shaderConstants.BindTextureUAV(7, registry.GetTextureUAVBindlessResourceDescriptorIndex(indirectDiffuseTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::ScreenSpaceIndirectDiffuse);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        sceneTextures.indirectDiffuseTexture = indirectDiffuseTexture;
    }
}