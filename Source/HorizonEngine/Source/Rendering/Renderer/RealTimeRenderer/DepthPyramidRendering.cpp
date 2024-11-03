#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::RenderDepthPyramid(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        const RenderGraphTextureDesc& depthPyramidTextureDesc = sceneTextures.depthPyramidTextureDesc;

        renderGraph.AddPass(
            std::format("BuildDepthPyramid-Min (Compute, {}x{})", depthPyramidTextureDesc.width, depthPyramidTextureDesc.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle minDepthPyramidTexture = sceneTextures.minDepthPyramidTexture = builder.WriteTexture(sceneTextures.minDepthPyramidTexture, RenderBackendResourceState::UnorderedAccess);
                //RenderGraphTextureHandle maxDepthPyramidTexture = sceneTextures.maxDepthPyramidTexture = builder.WriteTexture(sceneTextures.maxDepthPyramidTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::BuildDepthPyramid);

                    // Build first mip
                    {
                        uint32 sourceMipLevel = 0;
                        uint32 targetMipLevel = 0;
                        Vector2u inputTextureSize = Vector2u(renderResolution.width, renderResolution.height);
                        Vector2u outputTextureSize = Vector2u(depthPyramidTextureDesc.width, depthPyramidTextureDesc.height);
                        Vector2 inverseInputTextureSize = Vector2(1.0f / float(inputTextureSize.x), 1.0f / float(inputTextureSize.y));
                        Vector2 inverseOutputTextureSize = Vector2(1.0f / float(outputTextureSize.x), 1.0f / float(outputTextureSize.y));

                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindTextureSRV(0, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                        shaderConstants.BindTextureUAV(1, registry.GetTextureUAVBindlessResourceDescriptorIndex(minDepthPyramidTexture, targetMipLevel));
                        shaderConstants.BindScalar(3, inverseInputTextureSize.x);
                        shaderConstants.BindScalar(4, inverseInputTextureSize.y);

                        uint32 threadGroupCountX = CeilDiv(outputTextureSize.x, 8);
                        uint32 threadGroupCountY = CeilDiv(outputTextureSize.y, 8);
                        uint32 threadGroupCountZ = 1;

                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    }

                    // Build other mips
                    for (uint32 mipLevel = 1; mipLevel < depthPyramidTextureDesc.mipLevelCount; mipLevel++)
                    {
                        uint32 sourceMipLevel = mipLevel - 1;
                        uint32 targetMipLevel = mipLevel;
                        Vector2u inputTextureSize = Vector2u(depthPyramidTextureDesc.width >> (mipLevel - 1), depthPyramidTextureDesc.height >> (mipLevel - 1));
                        Vector2u outputTextureSize = Vector2u(depthPyramidTextureDesc.width >> mipLevel, depthPyramidTextureDesc.height >> mipLevel);
                        Vector2 inverseInputTextureSize = Vector2(1.0f / float(inputTextureSize.x), 1.0f / float(inputTextureSize.y));
                        Vector2 inverseOutputTextureSize = Vector2(1.0f / float(outputTextureSize.x), 1.0f / float(outputTextureSize.y));

                        std::vector<RenderBackendBarrier> transitions;
                        //transitions.emplace_back(RenderBackendBarrier(registry.GetRenderBackendTextureHandle(closestHZBTexture), RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, 1), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource));
                        transitions.emplace_back(RenderBackendBarrier(registry.GetRenderBackendTextureHandle(minDepthPyramidTexture), RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, 1), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource));
                        commandList.Transitions(transitions.data(), (uint32)transitions.size());

                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindTextureSRV(0, registry.GetTextureSRVBindlessResourceDescriptorIndex(minDepthPyramidTexture, sourceMipLevel));
                        shaderConstants.BindTextureUAV(1, registry.GetTextureUAVBindlessResourceDescriptorIndex(minDepthPyramidTexture, targetMipLevel));
                        shaderConstants.BindScalar(3, inverseInputTextureSize.x);
                        shaderConstants.BindScalar(4, inverseInputTextureSize.y);

                        uint32 threadGroupCountX = CeilDiv(outputTextureSize.x, 8);
                        uint32 threadGroupCountY = CeilDiv(outputTextureSize.y, 8);
                        uint32 threadGroupCountZ = 1;

                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    }

                    // TODO: Find better way to do this
                    std::vector<RenderBackendBarrier> transitions;
                    //transitions.emplace_back(RenderBackendBarrier(registry.GetRenderBackendTextureHandle(closestHZBTexture), RenderBackendTextureSubresourceRange(0, std::max(hzbMipLevels - 1, 0u), 0, 1), RenderBackendResourceState::ShaderResource, RenderBackendResourceState::UnorderedAccess));
                    transitions.emplace_back(RenderBackendBarrier(registry.GetRenderBackendTextureHandle(minDepthPyramidTexture), RenderBackendTextureSubresourceRange(0, std::max(depthPyramidTextureDesc.mipLevelCount - 1, 0u), 0, 1), RenderBackendResourceState::ShaderResource, RenderBackendResourceState::UnorderedAccess));
                    commandList.Transitions(transitions.data(), (uint32)transitions.size());
                };
            });
    }
}