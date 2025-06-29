#include "RasterizationRenderer.h"

namespace Horizon
{
    void RasterizationRenderer::DispatchDepthPyramidGeneration(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "DepthPyramidGeneration");

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        // Hierarchical z-buffer must be aligned quad tree.
        uint32 depthPyramidTextureWidth = Math::Max(Math::RoundUpToPowerOfTwo(renderResolution.width) >> 1, 1u);
        uint32 depthPyramidTextureHeight = Math::Max(Math::RoundUpToPowerOfTwo(renderResolution.height) >> 1, 1u);
        uint32 depthPyramidTextureMipLevelCount = Math::MaxMipLevelCount(depthPyramidTextureWidth, depthPyramidTextureHeight);

        RenderGraphTextureDescription depthPyramidTextureDesc = RenderGraphTextureDescription::Create2D(
            depthPyramidTextureWidth,
            depthPyramidTextureHeight,
            RenderBackendTextureFormat::R32Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource,
            RenderBackendTextureClearValue::DepthZero,
            depthPyramidTextureMipLevelCount);

        intermediateResources.minDepthPyramidTexture = renderGraph.CreateTexture(depthPyramidTextureDesc, "MinDepthPyramidTexture");
        intermediateResources.maxDepthPyramidTexture = renderGraph.CreateTexture(depthPyramidTextureDesc, "MaxDepthPyramidTexture");

        renderGraph.AddPass(
            std::format("BuildDepthPyramid-Min (Compute, {}x{})", depthPyramidTextureDesc.width, depthPyramidTextureDesc.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(intermediateResources.depthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle minDepthPyramidTexture = intermediateResources.minDepthPyramidTexture = builder.WriteTexture(intermediateResources.minDepthPyramidTexture, RenderBackendResourceState::UnorderedAccess);
                //RenderGraphTextureHandle maxDepthPyramidTexture = intermediateResources.maxDepthPyramidTexture = builder.WriteTexture(intermediateResources.maxDepthPyramidTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::BuildDepthPyramid);

                    // Build first mip
                    {
                        uint32 sourceMipLevel = 0;
                        uint32 targetMipLevel = 0;
                        Vector2u inputTextureSize = Vector2u(renderResolution.width, renderResolution.height);
                        Vector2u outputTextureSize = Vector2u(depthPyramidTextureDesc.width, depthPyramidTextureDesc.height);
                        Vector2f inverseInputTextureSize = Vector2f(1.0f / float(inputTextureSize.x), 1.0f / float(inputTextureSize.y));
                        Vector2f inverseOutputTextureSize = Vector2f(1.0f / float(outputTextureSize.x), 1.0f / float(outputTextureSize.y));

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                        pushConstantValues.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(minDepthPyramidTexture, targetMipLevel));
                        pushConstantValues.BindScalar(3, inverseInputTextureSize.x);
                        pushConstantValues.BindScalar(4, inverseInputTextureSize.y);

                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(outputTextureSize.x, 8);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(outputTextureSize.y, 8);
                        uint32 threadGroupCountZ = 1;

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
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
                        Vector2f inverseInputTextureSize = Vector2f(1.0f / float(inputTextureSize.x), 1.0f / float(inputTextureSize.y));
                        Vector2f inverseOutputTextureSize = Vector2f(1.0f / float(outputTextureSize.x), 1.0f / float(outputTextureSize.y));

                        std::vector<RenderBackendBarrier> transitions;
                        //barriers.emplace_back(RenderBackendBarrier(resourceRegistry.GetRenderBackendTextureHandle(closestHZBTexture), RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, 1), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource));
                        transitions.emplace_back(RenderBackendBarrier(resourceRegistry.GetRenderBackendTextureHandle(minDepthPyramidTexture), RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, 1), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource));
                        commandList.Barriers(transitions.data(), (uint32)transitions.size());

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(minDepthPyramidTexture, sourceMipLevel));
                        pushConstantValues.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(minDepthPyramidTexture, targetMipLevel));
                        pushConstantValues.BindScalar(3, inverseInputTextureSize.x);
                        pushConstantValues.BindScalar(4, inverseInputTextureSize.y);

                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(outputTextureSize.x, 8);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(outputTextureSize.y, 8);
                        uint32 threadGroupCountZ = 1;

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    }

                    // TODO: Find better way to do this
                    std::vector<RenderBackendBarrier> transitions;
                    //barriers.emplace_back(RenderBackendBarrier(resourceRegistry.GetRenderBackendTextureHandle(closestHZBTexture), RenderBackendTextureSubresourceRange(0, std::max(hzbMipLevels - 1, 0u), 0, 1), RenderBackendResourceState::ShaderResource, RenderBackendResourceState::UnorderedAccess));
                    transitions.emplace_back(RenderBackendBarrier(resourceRegistry.GetRenderBackendTextureHandle(minDepthPyramidTexture), RenderBackendTextureSubresourceRange(0, std::max(depthPyramidTextureDesc.mipLevelCount - 1, 0u), 0, 1), RenderBackendResourceState::ShaderResource, RenderBackendResourceState::UnorderedAccess));
                    commandList.Barriers(transitions.data(), (uint32)transitions.size());
                };
            });
    }
}