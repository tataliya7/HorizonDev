#include "RealTimeRenderer.h"

namespace Horizon
{
    bool RealTimeRenderer::IsScreenSpaceAmbientOcclusionEnabled() const
    {
        return features.enableScreenSpaceAmbientOcclusion;
    }

    RenderGraphTextureHandle RealTimeRenderer::RenderScreenSpaceAmbientOcclusion(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        return RenderGraphTextureHandle::Null;
//         const auto& settings = this->settings.gtaoSettings;
//
//         uint32 downsampleFactor = 1;
//         uint32 gtaoTextureWidth = ComputeWorkGroupCount(renderResolution.width, downsampleFactor);
//         uint32 gtaoTextureHeight = ComputeWorkGroupCount(renderResolution.height, downsampleFactor);
//
//         //RenderBackendTextureFormat ambientOcclusionTextureFormat = RenderBackendTextureFormat::R8Unorm;
//         RenderBackendTextureFormat ambientOcclusionTextureFormat = RenderBackendTextureFormat::R16Float;
//
//         RenderGraphTextureDesc ambientOcclusionTextureDesc = RenderGraphTextureDesc::Create2D(
//             renderResolution.width,
//             renderResolution.height,
//             ambientOcclusionTextureFormat,
//             RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
//         RenderGraphTextureHandle ambientOcclusionTexture = renderGraph.CreateTexture(ambientOcclusionTextureDesc, "AmbientOcclusionTexture");
//
//         RenderGraphTextureDesc gtaoTextureDesc = RenderGraphTextureDesc::Create2D(
//             gtaoTextureWidth,
//             gtaoTextureHeight,
//             ambientOcclusionTextureFormat,
//             RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
//         RenderGraphTextureHandle gtaoHorizonSearchAndIntegralTexture = renderGraph.CreateTexture(gtaoTextureDesc, "GTAOHorizonSearchAndIntegralTexture");
//         RenderGraphTextureHandle gtaoSpatialFilteringTexture = renderGraph.CreateTexture(gtaoTextureDesc, "GTAOSpatialFilteringTexture");
//         RenderGraphTextureHandle gtaoTemporalFilteringTexture = renderGraph.CreateTexture(gtaoTextureDesc, "GTAOTemporalFilteringTexture");
//
//         RenderGraphTextureHandle gtaoDebugOutputTexture = renderGraph.CreateTexture(gtaoTextureDesc, "GTAODebugOutputTexture");
//
//         auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
//
//         renderGraph.AddPass(std::format("GTAOHorizonSearchAndIntegral (Compute, {}, {})", gtaoTextureWidth, gtaoTextureHeight), RenderGraphPassFlags::AsyncCompute,
//             [&](RenderGraphBuilder& builder)
//             {
//                 auto gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
//                 auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
//
//                 gtaoHorizonSearchAndIntegralTexture = builder.WriteTexture(gtaoHorizonSearchAndIntegralTexture, RenderBackendResourceState::UnorderedAccess);
//                 gtaoDebugOutputTexture = builder.WriteTexture(gtaoDebugOutputTexture, RenderBackendResourceState::UnorderedAccess);
//
//                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                 {
//                     uint32 threadGroupCountX = ComputeWorkGroupCount(gtaoTextureWidth, 8);
//                     uint32 threadGroupCountY = ComputeWorkGroupCount(gtaoTextureHeight, 8);
//
//                     RenderBackendShaderConstants shaderConstants = {};
//                     shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
//                     shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture)));
//                     shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0)));
//                     shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndexgtaoHorizonSearchAndIntegralTexture), 0));
//                     shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndexgtaoDebugOutputTexture), 0));
//                     shaderConstants.PushConstants(0, 1.0f / gtaoTextureWidth);
//                     shaderConstants.PushConstants(1, 1.0f / gtaoTextureHeight);
//                     shaderConstants.PushConstants(2, settings.radius);
//                     shaderConstants.PushConstants(3, settings.thickness);
//                     shaderConstants.PushConstants(4, 100.0f);
//
//                     commandList.ClearTextureUAV(registry.GetTextureUAVBindlessResourceDescriptorIndexgtaoDebugOutputTexture), 0), RenderBackendTextureClearValue::CreateColorValueFloat4(0.0f, 0.0f, 0.0f, 0.0f));
//
//                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::GTAOHorizonSearchAndIntegral);
//                     commandList.Dispatch2D(
//                         computeShader,
//                         shaderConstants,
//                         threadGroupCountX,
//                         groupCountY);
//                 };
//             });
//
//         renderGraph.AddPass(std::format("GTAOSpatialFiltering (Compute, {}x{})", gtaoTextureWidth, gtaoTextureHeight), RenderGraphPassFlags::AsyncCompute,
//             [&](RenderGraphBuilder& builder)
//             {
//                 auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
//                 builder.ReadTexture(gtaoHorizonSearchAndIntegralTexture, RenderBackendResourceState::ShaderResource);
//
//                 gtaoSpatialFilteringTexture = builder.WriteTexture(gtaoSpatialFilteringTexture, RenderBackendResourceState::UnorderedAccess);
//
//                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                 {
//                     uint32 threadGroupCountX = ComputeWorkGroupCount(gtaoTextureWidth, 8);
//                     uint32 threadGroupCountY = ComputeWorkGroupCount(gtaoTextureHeight, 8);
//
//                     RenderBackendShaderConstants shaderConstants = {};
//                     shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
//                     shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture)));
//                     shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gtaoHorizonSearchAndIntegralTexture)));
//                     shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndexgtaoSpatialFilteringTexture), 0));
//                     shaderConstants.PushConstants(0, 1.0f / float(gtaoTextureWidth));
//                     shaderConstants.PushConstants(1, 0.0f);
//
//                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::GTAOSpatialFiltering);
//                     commandList.Dispatch2D(
//                         computeShader,
//                         shaderConstants,
//                         threadGroupCountX,
//                         groupCountY);
//                 };
//             });
//
//         /*   uint32 deviceMask = ~0u;
//         if (!historyAmbientOcclusionTextureCache.texture || (historyAmbientOcclusionTextureCache.desc != ambientOcclusionTextureDesc))
//         {
//             historyAmbientOcclusionTextureCache.desc = ambientOcclusionTextureDesc;
//             historyAmbientOcclusionTextureCache.texture = renderBackend->CreateTexture(deviceMask, &historyAmbientOcclusionTextureCache.desc, nullptr, "HistoryAmbientOcclusionTexture");
//             historyAmbientOcclusionTextureCache.initialState = RenderBackendResourceState::ShaderResource;
//         }
//         auto historyAmbientOcclusionTexture = renderGraph.ImportExternalTexture(historyAmbientOcclusionTextureCache.texture, historyAmbientOcclusionTextureCache.desc, historyAmbientOcclusionTextureCache.initialState, "HistoryAmbientOcclusionTexture");
//         renderGraph.ExportTextureDeferred(ambientOcclusionTexture, &historyAmbientOcclusionTextureCache);
//
//         renderGraph.AddPass(std::format("GTAOTemporalFiltering (Compute, {}x{})", ambientOcclusionTextureWidth, ambientOcclusionTextureHeight), RenderGraphPassFlags::AsyncCompute,
//             [&](RenderGraphBuilder& builder)
//             {
//                 auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
//                 auto& historyInfo = renderGraph.blackboard.Get<RealTimeRendererHistoryInfo>();
//
//                 auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
//                 auto historySceneDepthTexture = builder.ReadTexture(historyInfo.historySceneDepthTexture, RenderBackendResourceState::ShaderResource);
//                 auto motionVectorTexture = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);
//                 builder.ReadTexture(historyAmbientOcclusionTexture, RenderBackendResourceState::ShaderResource);
//                 builder.ReadTexture(horizonSearchAndIntegralTexture, RenderBackendResourceState::ShaderResource);
//
//                 ambientOcclusionTexture = builder.WriteTexture(ambientOcclusionTexture, RenderBackendResourceState::UnorderedAccess);
//
//                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                 {
//                     uint32 threadGroupCountX = ComputeWorkGroupCount(ambientOcclusionTextureWidth, 8);
//                     uint32 threadGroupCountY = ComputeWorkGroupCount(ambientOcclusionTextureHeight, 8);
//
//                     RenderBackendShaderConstants shaderConstants = {};
//                     shaderConstants.BindBufferCBV(0, renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
//                     shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture)));
//                     shaderConstants.BindTextureSRV(7, registry.GetTextureSRVBindlessResourceDescriptorIndex(motionVectorTexture)));
//                     shaderConstants.BindTextureSRV(8, registry.GetTextureSRVBindlessResourceDescriptorIndex(historySceneDepthTexture)));
//                     shaderConstants.BindTextureSRV(9, registry.GetTextureSRVBindlessResourceDescriptorIndex(horizonSearchAndIntegralTexture)));
//                     shaderConstants.BindTextureSRV(10, registry.GetTextureSRVBindlessResourceDescriptorIndex(historyAmbientOcclusionTexture)));
//                     shaderConstants.BindTextureUAV(11, registry.GetTextureUAVBindlessResourceDescriptorIndexambientOcclusionTexture), 0));
//
//                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::GTAOTemporalFiltering);
//                     commandList.Dispatch2D(
//                         computeShader,
//                         shaderConstants,
//                         threadGroupCountX,
//                         groupCountY);
//                 };
//             });*/
//
//         ambientOcclusionTexture = gtaoSpatialFilteringTexture;
//
//         return ambientOcclusionTexture;
    }
}