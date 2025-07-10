#include "../RasterizationRenderer.h"
#include "PostProcessing.h"

/**
 * Jump flood algorithm based outline
 * See: https://bgolus.medium.com/the-quest-for-very-wide-outlines-ba82ed442cd9
 * Source code: https://gist.github.com/bgolus/a18c1a3fc9af2d73cc19169a809eb195
 */

// TODO:
// 1. Use separable axis method
// 2. Optimize the render target format
// 3. Anti-aliased outline

namespace Horizon
{
    RenderGraphTextureHandle RasterizationRenderer::DispatchEditorSelectionOutline(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "EditorSelectionOutline");

        RenderGraphTextureDescription maskTextureDesc = RenderGraphTextureDescription::Create2D(
            targetResolution.width,
            targetResolution.height,
            RenderBackendTextureFormat::R8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget,
            RenderBackendTextureClearValue::Black);
        RenderGraphTextureHandle maskTexture = renderGraph.CreateTexture(maskTextureDesc, "SelectionOutlineMaskTexture");

        renderGraph.AddPass(
            std::format("SelectionOutlineMask (Graphics, {}x{})", targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetRenderTargetBinding(0, maskTexture, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Store);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendViewport viewport(0.0f, 0.0f, (float)targetResolution.width, (float)targetResolution.height);
                        commandList.SetViewports(&viewport, 1);

                        RenderBackendScissor scissor(0, 0, targetResolution.width, targetResolution.height);
                        commandList.SetScissors(&scissor, 1);

                        RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
                        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;

                        /*for (const auto& drawCallInfo : renderEngine->drawList)
                        {
                            RenderBackendPushConstantValues pushConstantValues = {};
                            pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                            pushConstantValues.BindBuffer(1, renderEngine->geometryBuffer, drawCallInfo.geometryIndex * sizeof(GeometryShaderParameters));
                            pushConstantValues.BindBuffer(2, drawCallInfo.vertexBuffers[0], 0);

                            RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::SelectionOutlineMaskGen);
                            commandList.DrawIndexed(
                                graphicsShader,
                                graphicsPipelineState,
                                pushConstantValues,
                                drawCallInfo.indexBuffer,
                                drawCallInfo.indexCount,
                                1,
                                drawCallInfo.firstIndex,
                                0,
                                0,
                                RenderBackendPrimitiveTopology::TriangleList);
                        }*/
                };
            });

        RenderGraphTextureDescription jumpFloodTextureDesc = RenderGraphTextureDescription::Create2D(
            targetResolution.width,
            targetResolution.height,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle jumpFloodTexture0 = renderGraph.CreateTexture(jumpFloodTextureDesc, "SelectionOutlineJumpFloodTexture0");
        RenderGraphTextureHandle jumpFloodTexture1 = renderGraph.CreateTexture(jumpFloodTextureDesc, "SelectionOutlineJumpFloodTexture1");

        renderGraph.AddPass(
            std::format("SelectionOutlineSetup (Compute, {}x{})", targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                maskTexture = builder.ReadTexture(maskTexture, RenderBackendResourceState::ShaderResource);
                jumpFloodTexture0 = builder.WriteTexture(jumpFloodTexture0, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(maskTexture));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(jumpFloodTexture0, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SelectionOutlineSetup);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        RenderGraphTextureHandle jumpFloodTexture = jumpFloodTexture0;

        // TODO: promoting parameters
        float outlineWidth = 2.0f;
        uint32 numSteps = (uint32)std::ceil(std::log(outlineWidth + 1.0f));

        RenderGraphTextureHandle jumpFloodPassInputTexture = jumpFloodTexture0;
        RenderGraphTextureHandle jumpFloodPassOutputTexture = jumpFloodTexture1;

        for (uint32 step = numSteps; step > 0; step--)
        {
            float stepWidth = std::pow(2.0f, (float)step);

            renderGraph.AddPass(
                std::format("SelectionOutlineJumpFlood-Step{} (Compute, {}x{}, StepWidth={})", step, targetResolution.width, targetResolution.height, stepWidth),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    jumpFloodPassInputTexture = builder.ReadTexture(jumpFloodPassInputTexture, RenderBackendResourceState::ShaderResource);
                    jumpFloodPassOutputTexture = builder.WriteTexture(jumpFloodPassOutputTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                        uint32 threadGroupCountZ = 1;

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(jumpFloodPassInputTexture));
                        pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(jumpFloodPassOutputTexture, 0));
                        pushConstantValues.BindScalar(3, stepWidth);

                        RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SelectionOutlineJumpFlood);

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    };
                });

            jumpFloodTexture = jumpFloodPassOutputTexture;
            std::swap(jumpFloodPassInputTexture, jumpFloodPassOutputTexture);
        }

        RenderGraphTextureHandle outputTexture = sceneColorTexture;

        renderGraph.AddPass(
            std::format("SelectionOutlineComposite (Compute, {}x{})", targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                jumpFloodTexture = builder.ReadTexture(jumpFloodTexture, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(jumpFloodTexture));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SelectionOutlineComposite);

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
}