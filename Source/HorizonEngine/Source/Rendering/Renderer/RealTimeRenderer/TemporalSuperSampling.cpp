#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::AddTemporalSuperSamplingPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        RenderGraphTextureHandle sceneDepthTexture,
        RenderGraphTextureHandle motionVectorTexture)
    {
        uint32 deviceMask = ~0u;

        RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolutionX,
            targetResolutionY,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "TemporalSuperSamplingTexture");

        bool invalidHistory = false;
        if (!historyTemporalSuperSamplingTextureCache.texture || (historyTemporalSuperSamplingTextureCache.desc != outputTextureDesc))
        {
            // TODO: destroy previous resource
            historyTemporalSuperSamplingTextureCache.desc = outputTextureDesc;
            historyTemporalSuperSamplingTextureCache.texture = renderBackend->CreateTexture(deviceMask, &historyTemporalSuperSamplingTextureCache.desc, nullptr, "HistoryTemporalSuperSamplingTexture");
            historyTemporalSuperSamplingTextureCache.initialState = RenderBackendResourceState::ShaderResource;
            invalidHistory = true;
        }
        RenderGraphTextureHandle historyTemporalSuperSamplingTexture = renderGraph.ImportExternalTexture(historyTemporalSuperSamplingTextureCache.texture, historyTemporalSuperSamplingTextureCache.desc, historyTemporalSuperSamplingTextureCache.initialState, "HistoryTemporalSuperSamplingTexture");
        renderGraph.ExportTextureDeferred(historyTemporalSuperSamplingTexture, &historyTemporalSuperSamplingTextureCache);

        float reset = 0.0f;
        if (sceneViewShaderParameters.frameIndex == 0 || invalidHistory)
        {
            reset = 1.0f;
        }

        renderGraph.AddPass("TemporalSuperSampling", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
                auto& historyInfo = renderGraph.blackboard.Get<RealTimeRendererHistoryInfo>();

                auto sceneColorTexture = builder.ReadTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::ShaderResource);
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                auto motionVectorTexture = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);
                auto historySceneDepth = builder.ReadTexture(historyInfo.historySceneDepthTexture, RenderBackendResourceState::ShaderResource);
                historyTemporalSuperSamplingTexture = builder.ReadTexture(historyTemporalSuperSamplingTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(targetResolutionX, 8);
                    uint32 groupCountY = ComputeWorkGroupCount(targetResolutionY, 8);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(motionVectorTexture)));
                    shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(historySceneDepth)));
                    shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(historyTemporalSuperSamplingTexture)));
                    shaderArguments.BindTextureUAV(6, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(outputTexture), 0));
                    shaderArguments.PushConstants(0, reset);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::TemporalSuperSampling);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
                };
            });

        return outputTexture;
    }
}