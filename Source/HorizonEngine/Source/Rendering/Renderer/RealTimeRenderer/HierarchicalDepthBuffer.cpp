#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::RenderHZB(
        RenderGraph& renderGraph,
        const SceneView& view,
        uint32 hzbWidth,
        uint32 hzbHeight,
        uint32 hzbMipLevels,
        RenderGraphTextureHandle& closestHZBTexture,
        RenderGraphTextureHandle& furthestHZBTexture)
    {
        auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        renderGraph.AddPass(std::format("BuildHZB-Closets&Furthest (Compute, {}x{})", hzbWidth, hzbHeight), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);

                closestHZBTexture = builder.WriteTexture(closestHZBTexture, RenderBackendResourceState::UnorderedAccess);
                furthestHZBTexture = builder.WriteTexture(furthestHZBTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderHandle buildHZBCS = shaderLibrary->GetShaderHandle(ShaderID::BuildHZB);

                    // Build first mip
                    {
                        uint32 srcMip = 0;
                        uint32 dstMip = 0;
                        Vector2u srcSize = Vector2u(renderResolutionX, renderResolutionY);
                        Vector2u dstSize = Vector2u(hzbWidth, hzbHeight);
                        Vector2 invSrcSize = Vector2(1.0f / srcSize.x, 1.0f / srcSize.y);
                        Vector2 invDstSize = Vector2(1.0f / dstSize.x, 1.0f / dstSize.y);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                        shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(closestHZBTexture), dstMip));
                        shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(furthestHZBTexture), dstMip));
                        shaderArguments.PushConstants(0, invSrcSize.x);
                        shaderArguments.PushConstants(1, invSrcSize.y);

                        uint32 groupCountX = ComputeWorkGroupCount(dstSize.x, 8);
                        uint32 groupCountY = ComputeWorkGroupCount(dstSize.y, 8);

                        commandList.Dispatch2D(
                            buildHZBCS,
                            shaderArguments,
                            groupCountX,
                            groupCountY);
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

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::CreateForMipLevel(registry.GetRenderBackendTextureHandle(furthestHZBTexture), mipLevel - 1));
                        shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(closestHZBTexture), dstMip));
                        shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(furthestHZBTexture), dstMip));
                        shaderArguments.PushConstants(0, invSrcSize.x);
                        shaderArguments.PushConstants(1, invSrcSize.y);

                        uint32 groupCountX = ComputeWorkGroupCount(dstSize.x, 8);
                        uint32 groupCountY = ComputeWorkGroupCount(dstSize.y, 8);

                        commandList.Dispatch2D(
                            buildHZBCS,
                            shaderArguments,
                            groupCountX,
                            groupCountY);
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