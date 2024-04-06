#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace HE
{
    RenderGraphTextureHandle RealTimeRenderer::AddDownsamplePass(
        RenderGraph& renderGraph,
        const SceneView& view,
        uint32 inputTextureWidth,
        uint32 inputTextureHeight,
        uint32 outputTextureWidth,
        uint32 outputTextureHeight,
        RenderGraphTextureHandle inputTexture,
        RenderGraphTextureHandle outputTexture)
    {
        renderGraph.AddPass(std::format("Downsample (Compute, {}x{} -> {}x{})", inputTextureWidth, inputTextureHeight, outputTextureWidth, outputTextureHeight), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                inputTexture = builder.ReadTexture(inputTexture, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 dispatchX = Math::CeilDiv(outputTextureWidth, PostProcessingThreadGroupCountX);
                    uint32 dispatchY = Math::CeilDiv(outputTextureHeight, PostProcessingThreadGroupCountY);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(inputTexture)));
                    shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(outputTexture), 0));

                    auto downsampleCS = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::Downsample);
                    commandList.Dispatch2D(
                        downsampleCS,
                        shaderArguments,
                        dispatchX,
                        dispatchY);
                };
            });

        return outputTexture;
    }

    void RealTimeRenderer::AddGenerateSceneColorMipChainPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        PostProcessingSceneColorMipChain* outMipChain)
    {
        RenderGraphTextureHandle inputTexture = sceneColorTexture;
        uint32 inputTextureWidth = targetResolutionX;
        uint32 inputTextureHeight = targetResolutionY;
        for (uint32 i = 0; i < 6; i++)
        {
            RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
                inputTextureWidth / 2,
                inputTextureHeight / 2,
                RenderBackendTextureFormat::R11G11B10Float,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
            RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "DownsampledSceneColorTexture");
            outMipChain->textures[i] = AddDownsamplePass(renderGraph, view, inputTextureWidth, inputTextureHeight, inputTextureWidth / 2, inputTextureHeight / 2, inputTexture, outputTexture);
            inputTexture = outMipChain->textures[i];
            inputTextureWidth = inputTextureWidth / 2;
            inputTextureHeight = inputTextureHeight / 2;
        }
    }
}