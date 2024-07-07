#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::RenderDepthPyramid(
        RenderGraph& renderGraph,
        const SceneView& view,
        uint32 hzbWidth,
        uint32 hzbHeight,
        uint32 hzbMipLevels,
        RenderGraphTextureHandle& closestHZBTexture,
        RenderGraphTextureHandle& furthestHZBTexture)
    {
        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        renderGraph.AddPass(
            std::format("BuildDepthPyramid-Closets&Furthest (Compute, {}x{})", hzbWidth, hzbHeight),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                closestHZBTexture = builder.WriteTexture(closestHZBTexture, RenderBackendResourceState::UnorderedAccess);
                furthestHZBTexture = builder.WriteTexture(furthestHZBTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::BuildDepthPyramid);

                    // Build first mip
                    {
                        uint32 srcMip = 0;
                        uint32 dstMip = 0;
                        Vector2u srcSize = Vector2u(renderResolution.width, renderResolution.height);
                        Vector2u dstSize = Vector2u(hzbWidth, hzbHeight);
                        Vector2 invSrcSize = Vector2(1.0f / srcSize.x, 1.0f / srcSize.y);
                        Vector2 invDstSize = Vector2(1.0f / dstSize.x, 1.0f / dstSize.y);

                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindTextureSRV(0, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                        shaderConstants.BindTextureUAV(1, registry.GetTextureUAVBindlessResourceDescriptorIndex(closestHZBTexture, dstMip));
                        shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(furthestHZBTexture, dstMip));
                        shaderConstants.BindScalar(3, invSrcSize.x);
                        shaderConstants.BindScalar(4, invSrcSize.y);

                        uint32 threadGroupCountX = CeilDiv(dstSize.x, 8);
                        uint32 threadGroupCountY = CeilDiv(dstSize.y, 8);
                        uint32 threadGroupCountZ = 1;

                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    }

                    // Build other mips
                    for (uint32 mipLevel = 1; mipLevel < hzbMipLevels; mipLevel++)
                    {
                        uint32 srcMip = mipLevel - 1;
                        uint32 dstMip = mipLevel;
                        Vector2u srcSize = Vector2u(hzbWidth >> (mipLevel - 1), hzbHeight >> (mipLevel - 1));
                        Vector2u dstSize = Vector2u(hzbWidth >> mipLevel, hzbHeight >> mipLevel);
                        Vector2 invSrcSize = Vector2(1.0f / srcSize.x, 1.0f / srcSize.y);
                        Vector2 invDstSize = Vector2(1.0f / dstSize.x, 1.0f / dstSize.y);

                        std::vector<RenderBackendBarrier> transitions;
                        transitions.emplace_back(RenderBackendBarrier(registry.GetRenderBackendTextureHandle(closestHZBTexture), RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, 1), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource));
                        transitions.emplace_back(RenderBackendBarrier(registry.GetRenderBackendTextureHandle(furthestHZBTexture), RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, 1), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource));
                        commandList.Transitions(transitions.data(), (uint32)transitions.size());

                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindTextureSRV(0, registry.GetTextureSRVBindlessResourceDescriptorIndex(furthestHZBTexture)); // TODO: specify mip level // mipLevel - 1
                        shaderConstants.BindTextureUAV(1, registry.GetTextureUAVBindlessResourceDescriptorIndex(closestHZBTexture, dstMip));
                        shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(furthestHZBTexture, dstMip));
                        shaderConstants.BindScalar(3, invSrcSize.x);
                        shaderConstants.BindScalar(4, invSrcSize.y);

                        uint32 threadGroupCountX = CeilDiv(dstSize.x, 8);
                        uint32 threadGroupCountY = CeilDiv(dstSize.y, 8);
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
                    transitions.emplace_back(RenderBackendBarrier(registry.GetRenderBackendTextureHandle(closestHZBTexture), RenderBackendTextureSubresourceRange(0, std::max(hzbMipLevels - 1, 0u), 0, 1), RenderBackendResourceState::ShaderResource, RenderBackendResourceState::UnorderedAccess));
                    transitions.emplace_back(RenderBackendBarrier(registry.GetRenderBackendTextureHandle(furthestHZBTexture), RenderBackendTextureSubresourceRange(0, std::max(hzbMipLevels - 1, 0u), 0, 1), RenderBackendResourceState::ShaderResource, RenderBackendResourceState::UnorderedAccess));
                    commandList.Transitions(transitions.data(), (uint32)transitions.size());
                };
            });
    }
}