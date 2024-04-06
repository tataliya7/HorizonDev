#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace HE
{
    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeShadowMaskPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture)
    {
        auto& debugViewModeTextures = renderGraph.blackboard.Get<RealTimeRendererDebugViewModeTextures>();
        const auto& finalTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();

        if (!debugViewModeTextures.screenSpaceShadowMaskTexture)
        {
            return sceneColorTexture;
        }

        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(finalTextureData.finalTextureDesc, "VisualizeScreenSpaceShadowMaskTexture");

        uint32 width = debugViewModeTextures.screenSpaceShadowMaskTextureDesc.width;
        uint32 height = debugViewModeTextures.screenSpaceShadowMaskTextureDesc.height;

        renderGraph.AddPass(std::format("VisualizeScreenSpaceShadowMask (Compute, {}x{})", width, height, targetResolutionX, targetResolutionY), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto screenSpaceShadowMaskTexture = builder.ReadTexture(debugViewModeTextures.screenSpaceShadowMaskTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 dispatchX = Math::CeilDiv(targetResolutionX, PostProcessingThreadGroupCountX);
                    uint32 dispatchY = Math::CeilDiv(targetResolutionY, PostProcessingThreadGroupCountY);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(screenSpaceShadowMaskTexture)));
                    shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(outputTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::VisualizeScreenSpaceShadowMask);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        dispatchX,
                        dispatchY);
                };
            });

        return outputTexture;
    }
}