#include "RealTimeRenderer.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::RenderScreenSpaceAmbientOcclusion(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const auto& settings = this->settings.gtaoSettings;

        uint32 downsampleFactor = 1;
        uint32 gtaoTextureWidth = ComputeWorkGroupCount(renderResolution.width, downsampleFactor);
        uint32 gtaoTextureHeight = ComputeWorkGroupCount(renderResolution.height, downsampleFactor);

        //RenderBackendTextureFormat ambientOcclusionTextureFormat = RenderBackendTextureFormat::R8Unorm;
        RenderBackendTextureFormat ambientOcclusionTextureFormat = RenderBackendTextureFormat::R16Float;

        RenderGraphTextureDesc ambientOcclusionTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            ambientOcclusionTextureFormat,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle ambientOcclusionTexture = renderGraph.CreateTexture(ambientOcclusionTextureDesc, "AmbientOcclusionTexture");

        RenderGraphTextureDesc gtaoTextureDesc = RenderGraphTextureDesc::Create2D(
            gtaoTextureWidth,
            gtaoTextureHeight,
            ambientOcclusionTextureFormat,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle gtaoHorizonSearchAndIntegralTexture = renderGraph.CreateTexture(gtaoTextureDesc, "GTAOHorizonSearchAndIntegralTexture");
        RenderGraphTextureHandle gtaoSpatialFilteringTexture = renderGraph.CreateTexture(gtaoTextureDesc, "GTAOSpatialFilteringTexture");
        RenderGraphTextureHandle gtaoTemporalFilteringTexture = renderGraph.CreateTexture(gtaoTextureDesc, "GTAOTemporalFilteringTexture");

        RenderGraphTextureHandle gtaoDebugOutputTexture = renderGraph.CreateTexture(gtaoTextureDesc, "GTAODebugOutputTexture");

        auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        renderGraph.AddPass(std::format("GTAOHorizonSearchAndIntegral (Compute, {}, {})", gtaoTextureWidth, gtaoTextureHeight), RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                auto gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);

                gtaoHorizonSearchAndIntegralTexture = builder.WriteTexture(gtaoHorizonSearchAndIntegralTexture, RenderBackendResourceState::UnorderedAccess);
                gtaoDebugOutputTexture = builder.WriteTexture(gtaoDebugOutputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(gtaoTextureWidth, 8);
                    uint32 groupCountY = ComputeWorkGroupCount(gtaoTextureHeight, 8);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer0)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(gtaoHorizonSearchAndIntegralTexture), 0));
                    shaderArguments.BindTextureUAV(4, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(gtaoDebugOutputTexture), 0));
                    shaderArguments.PushConstants(0, 1.0f / gtaoTextureWidth);
                    shaderArguments.PushConstants(1, 1.0f / gtaoTextureHeight);
                    shaderArguments.PushConstants(2, settings.radius);
                    shaderArguments.PushConstants(3, settings.thickness);
                    shaderArguments.PushConstants(4, 100.0f);

                    commandList.ClearTextureUAV(RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(gtaoDebugOutputTexture), 0), RenderBackendTextureClearValue::CreateColorValueFloat4(0.0f, 0.0f, 0.0f, 0.0f));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::GTAOHorizonSearchAndIntegral);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
                };
            });

        renderGraph.AddPass(std::format("GTAOSpatialFiltering (Compute, {}x{})", gtaoTextureWidth, gtaoTextureHeight), RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(gtaoHorizonSearchAndIntegralTexture, RenderBackendResourceState::ShaderResource);

                gtaoSpatialFilteringTexture = builder.WriteTexture(gtaoSpatialFilteringTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(gtaoTextureWidth, 8);
                    uint32 groupCountY = ComputeWorkGroupCount(gtaoTextureHeight, 8);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(gtaoHorizonSearchAndIntegralTexture)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(gtaoSpatialFilteringTexture), 0));
                    shaderArguments.PushConstants(0, 1.0f / float(gtaoTextureWidth));
                    shaderArguments.PushConstants(1, 0.0f);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::GTAOSpatialFiltering);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
                };
            });

        /*   uint32 deviceMask = ~0u;
        if (!historyAmbientOcclusionTextureCache.texture || (historyAmbientOcclusionTextureCache.desc != ambientOcclusionTextureDesc))
        {
            historyAmbientOcclusionTextureCache.desc = ambientOcclusionTextureDesc;
            historyAmbientOcclusionTextureCache.texture = renderBackend->CreateTexture(deviceMask, &historyAmbientOcclusionTextureCache.desc, nullptr, "HistoryAmbientOcclusionTexture");
            historyAmbientOcclusionTextureCache.initialState = RenderBackendResourceState::ShaderResource;
        }
        auto historyAmbientOcclusionTexture = renderGraph.ImportExternalTexture(historyAmbientOcclusionTextureCache.texture, historyAmbientOcclusionTextureCache.desc, historyAmbientOcclusionTextureCache.initialState, "HistoryAmbientOcclusionTexture");
        renderGraph.ExportTextureDeferred(ambientOcclusionTexture, &historyAmbientOcclusionTextureCache);

        renderGraph.AddPass(std::format("GTAOTemporalFiltering (Compute, {}x{})", ambientOcclusionTextureWidth, ambientOcclusionTextureHeight), RenderGraphPassFlags::AsyncCompute,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                auto& historyInfo = renderGraph.blackboard.Get<RealTimeRendererHistoryInfo>();

                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                auto historySceneDepthTexture = builder.ReadTexture(historyInfo.historySceneDepthTexture, RenderBackendResourceState::ShaderResource);
                auto motionVectorTexture = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(historyAmbientOcclusionTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(horizonSearchAndIntegralTexture, RenderBackendResourceState::ShaderResource);

                ambientOcclusionTexture = builder.WriteTexture(ambientOcclusionTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(ambientOcclusionTextureWidth, 8);
                    uint32 groupCountY = ComputeWorkGroupCount(ambientOcclusionTextureHeight, 8);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                    shaderArguments.BindTextureSRV(7, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(motionVectorTexture)));
                    shaderArguments.BindTextureSRV(8, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(historySceneDepthTexture)));
                    shaderArguments.BindTextureSRV(9, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(horizonSearchAndIntegralTexture)));
                    shaderArguments.BindTextureSRV(10, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(historyAmbientOcclusionTexture)));
                    shaderArguments.BindTextureUAV(11, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(ambientOcclusionTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::GTAOTemporalFiltering);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
                };
            });*/

        ambientOcclusionTexture = gtaoSpatialFilteringTexture;

        return ambientOcclusionTexture;
    }
}