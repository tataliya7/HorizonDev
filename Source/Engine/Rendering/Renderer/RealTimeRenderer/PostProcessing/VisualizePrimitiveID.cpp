#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace HE
{
    RenderGraphTextureHandle RealTimeRenderer::AddVisualizePrimitiveIDPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        const auto& finalTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();

        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(finalTextureData.finalTextureDesc, "VisualizePrimitiveIDTexture");

        renderGraph.AddPass(std::format("VisualizePrimitiveID (Compute, {}x{}->{}x{})", renderResolutionX, renderResolutionY, targetResolutionX, targetResolutionY), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 dispatchX = Math::CeilDiv(targetResolutionX, PostProcessingThreadGroupCountX);
                    uint32 dispatchY = Math::CeilDiv(targetResolutionY, PostProcessingThreadGroupCountY);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(vbuffer0)));
                    shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(outputTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::VisualizePrimitiveID);
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