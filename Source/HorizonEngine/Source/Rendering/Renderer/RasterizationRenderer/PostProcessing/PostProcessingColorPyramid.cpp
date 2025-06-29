#include "../RasterizationRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    RenderGraphTextureHandle RasterizationRenderer::AddDownsamplePass(
        RenderGraph& renderGraph,
        const SceneView& view,
        uint32 inputTextureWidth,
        uint32 inputTextureHeight,
        uint32 outputTextureWidth,
        uint32 outputTextureHeight,
        RenderGraphTextureHandle inputTexture,
        RenderGraphTextureHandle outputTexture)
    {
        renderGraph.AddPass(
            std::format("Downsample (Compute, {}x{} -> {}x{})", inputTextureWidth, inputTextureHeight, outputTextureWidth, outputTextureHeight),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                inputTexture = builder.ReadTexture(inputTexture, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(outputTextureWidth, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(outputTextureHeight, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(inputTexture));
                    pushConstantValues.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::BuildColorPyramid);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    void RasterizationRenderer::DispatchColorPyramidGeneration(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        PostProcessingColorPyramid* outMipChain)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "ColorPyramidGeneration");

        assert(outMipChain->mipLevelCount == 0);

        RenderGraphTextureHandle inputTexture = sceneColorTexture;
        uint32 inputTextureWidth = targetResolution.width;
        uint32 inputTextureHeight = targetResolution.height;

        for (uint32 passIndex = 0; passIndex < PostProcessingColorPyramid::MaxMipLevelCount; passIndex++)
        {
            RenderGraphTextureDescription outputTextureDesc = RenderGraphTextureDescription::Create2D(
                inputTextureWidth / 2,
                inputTextureHeight / 2,
                RenderBackendTextureFormat::R11G11B10Float,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);

            RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "DownsampledColorTexture");

            outputTexture = AddDownsamplePass(renderGraph, view, inputTextureWidth, inputTextureHeight, inputTextureWidth / 2, inputTextureHeight / 2, inputTexture, outputTexture);

            outMipChain->textures[passIndex] = outputTexture;
            outMipChain->mipLevelCount++;

            inputTexture = outMipChain->textures[passIndex];
            inputTextureWidth = inputTextureWidth / 2;
            inputTextureHeight = inputTextureHeight / 2;
        }
    }
}