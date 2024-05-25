#include "PostProcessingCommon.h"

namespace Horizon
{
    RenderGraphTextureHandle AddDownsamplePass(
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
                        uint32 groupCountX = ComputeWorkGroupCount(outputTextureWidth, 8);
                        uint32 groupCountY = ComputeWorkGroupCount(outputTextureHeight, 8);
                        uint32 groupCountZ = 1;

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(inputTexture)));
                        shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(outputTexture), 0));

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::Downsample);

                        commandList.Dispatch(
                            computeShader,
                            shaderArguments,
                            groupCountX,
                            groupCountY,
                            groupCountZ);
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
        assert(outMipChain->mipCount == 0);

        RenderGraphTextureHandle inputTexture = sceneColorTexture;
        uint32 inputTextureWidth = targetResolution.width;
        uint32 inputTextureHeight = targetResolution.height;

        outMipChain->textures[0] = sceneColorTexture;
        outMipChain->mipCount = 1;

        for (uint32 passIndex = 1; passIndex < PostProcessingSceneColorMipChain::MaxMipCount; passIndex++)
        {
            RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
                inputTextureWidth / 2,
                inputTextureHeight / 2,
                RenderBackendTextureFormat::R11G11B10Float,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
            RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "DownsampledSceneColorTexture");

            outMipChain->textures[passIndex] = AddDownsamplePass(renderGraph, view, inputTextureWidth, inputTextureHeight, inputTextureWidth / 2, inputTextureHeight / 2, inputTexture, outputTexture);
            outMipChain->mipCount++;

            inputTexture = outMipChain->textures[passIndex];
            inputTextureWidth = inputTextureWidth / 2;
            inputTextureHeight = inputTextureHeight / 2;
        }
    }
}