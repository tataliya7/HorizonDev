#include "../RealTimeRenderer.h"
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
    RenderGraphTextureHandle RealTimeRenderer::AddEditorSelectionOutlinePass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture)
    {
        RenderGraphTextureDesc maskTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolution.width,
            targetResolution.height,
            RenderBackendTextureFormat::R8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget,
            RenderBackendTextureClearValue::Black);
        RenderGraphTextureHandle maskTexture = renderGraph.CreateTexture(maskTextureDesc, "EditorSelectionOutlineMaskTexture");

        renderGraph.AddPass(
            std::format("EditorSelectionOutlineMask (Graphics, {}x{})", targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                maskTexture = builder.WriteTexture(maskTexture, RenderBackendResourceState::RenderTarget);

                builder.BindRenderTarget(0, maskTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        RenderBackendViewport viewport(0.0f, 0.0f, (float)targetResolution.width, (float)targetResolution.height);
                        commandList.SetViewports(&viewport, 1);

                        RenderBackendScissor scissor(0, 0, targetResolution.width, targetResolution.height);
                        commandList.SetScissors(&scissor, 1);

                        RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;

                        /*for (const auto& drawCallInfo : renderEngine->drawList)
                        {
                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                            shaderArguments.BindBuffer(1, renderEngine->geometryBuffer, drawCallInfo.geometryIndex * sizeof(GeometryShaderParameters));
                            shaderArguments.BindBuffer(2, drawCallInfo.vertexBuffers[0], 0);

                            RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::EditorSelectionOutlineMaskGen);
                            commandList.DrawIndexed(
                                graphicsShader,
                                graphicsPipelineState,
                                shaderArguments,
                                drawCallInfo.indexBuffer,
                                drawCallInfo.numIndices,
                                1,
                                drawCallInfo.firstIndex,
                                0,
                                0,
                                RenderBackendPrimitiveTopology::TriangleList);
                        }*/
                };
            });

        RenderGraphTextureDesc jumpFloodTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolution.width,
            targetResolution.height,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle jumpFloodTexture0 = renderGraph.CreateTexture(jumpFloodTextureDesc, "EditorSelectionOutlineJumpFloodTexture0");
        RenderGraphTextureHandle jumpFloodTexture1 = renderGraph.CreateTexture(jumpFloodTextureDesc, "EditorSelectionOutlineJumpFloodTexture1");

        renderGraph.AddPass(
            std::format("EditorSelectionOutlineSetup (Compute, {}x{})", targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                maskTexture = builder.ReadTexture(maskTexture, RenderBackendResourceState::ShaderResource);

                jumpFloodTexture0 = builder.WriteTexture(jumpFloodTexture0, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(maskTexture)));
                    shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(jumpFloodTexture0), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SelectionOutlineSetup);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
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
                std::format("EditorSelectionOutlineJumpFlood-Step{} (Compute, {}x{}, StepWidth={})", step, targetResolution.width, targetResolution.height, stepWidth),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    builder.ReadTexture(jumpFloodPassInputTexture, RenderBackendResourceState::ShaderResource);

                    jumpFloodPassOutputTexture = builder.WriteTexture(jumpFloodPassOutputTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 threadGroupCountX = ComputeThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                        uint32 threadGroupCountY = ComputeThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                        uint32 threadGroupCountZ = 1;

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(jumpFloodPassInputTexture)));
                        shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(jumpFloodPassOutputTexture), 0));
                        shaderArguments.PushConstants(0, (float)stepWidth);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SelectionOutlineJumpFlood);
                        commandList.Dispatch(
                            computeShader,
                            shaderArguments,
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
            std::format("EditorSelectionOutlineComposite (Compute, {}x{})", targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(jumpFloodTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(jumpFloodTexture)));
                    shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(outputTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SelectionOutlineComposite);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }
}