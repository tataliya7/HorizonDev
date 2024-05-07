#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

// Jump flood algorithm based outline
// See: https://bgolus.medium.com/the-quest-for-very-wide-outlines-ba82ed442cd9
// Github: https://gist.github.com/bgolus/a18c1a3fc9af2d73cc19169a809eb195

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
            targetResolutionX,
            targetResolutionY,
            RenderBackendTextureFormat::R8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget,
            RenderBackendTextureClearValue::Black);
        RenderGraphTextureHandle maskTexture = renderGraph.CreateTexture(maskTextureDesc, "EditorSelectionOutlineMaskTexture");

        renderGraph.AddPass(std::format("EditorSelectionOutlineMaskGen (Graphics, {}x{})", targetResolutionX, targetResolutionY), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                maskTexture = builder.WriteTexture(maskTexture, RenderBackendResourceState::RenderTarget);

                builder.BindColorTarget(0, maskTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, (float)targetResolutionX, (float)targetResolutionY);
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, targetResolutionX, targetResolutionY);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;

                    for (const auto& drawCallInfo : renderEngine->drawList)
                    {
                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                        shaderArguments.BindBuffer(1, renderEngine->geometryBuffer, drawCallInfo.geometryIndex * sizeof(GeometryShaderParameters));
                        shaderArguments.BindBuffer(2, drawCallInfo.vertexBuffers[0], 0);

                        RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle(ShaderID::EditorSelectionOutlineMaskGen);
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
                    }
                };
            });

        RenderGraphTextureDesc jumpFloodTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolutionX,
            targetResolutionY,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle jumpFloodTexture0 = renderGraph.CreateTexture(jumpFloodTextureDesc, "EditorSelectionOutlineJumpFloodTexture0");
        RenderGraphTextureHandle jumpFloodTexture1 = renderGraph.CreateTexture(jumpFloodTextureDesc, "EditorSelectionOutlineJumpFloodTexture1");

        renderGraph.AddPass(std::format("EditorSelectionOutlineSetup (Compute, {}x{})", targetResolutionX, targetResolutionY), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(maskTexture, RenderBackendResourceState::ShaderResource);

                jumpFloodTexture0 = builder.WriteTexture(jumpFloodTexture0, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(targetResolutionX, PostProcessingThreadGroupCountX);
                    uint32 groupCountY = ComputeWorkGroupCount(targetResolutionY, PostProcessingThreadGroupCountY);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(maskTexture)));
                    shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(jumpFloodTexture0), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::EditorSelectionOutlineSetup);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
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

            renderGraph.AddPass(std::format("EditorSelectionOutlineJumpFlood-Step{} (Compute, {}x{}, StepWidth={})", step, targetResolutionX, targetResolutionY, stepWidth), RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    builder.ReadTexture(jumpFloodPassInputTexture, RenderBackendResourceState::ShaderResource);

                    jumpFloodPassOutputTexture = builder.WriteTexture(jumpFloodPassOutputTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 groupCountX = ComputeWorkGroupCount(targetResolutionX, PostProcessingThreadGroupCountX);
                        uint32 groupCountY = ComputeWorkGroupCount(targetResolutionY, PostProcessingThreadGroupCountY);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(jumpFloodPassInputTexture)));
                        shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(jumpFloodPassOutputTexture), 0));
                        shaderArguments.PushConstants(0, (float)stepWidth);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::EditorSelectionOutlineJumpFlood);
                        commandList.Dispatch2D(
                            computeShader,
                            shaderArguments,
                            groupCountX,
                            groupCountY);
                    };
                });

            jumpFloodTexture = jumpFloodPassOutputTexture;
            std::swap(jumpFloodPassInputTexture, jumpFloodPassOutputTexture);
        }

        RenderGraphTextureHandle outputTexture = sceneColorTexture;

        renderGraph.AddPass(std::format("EditorSelectionOutlineComposite (Compute, {}x{})", targetResolutionX, targetResolutionY), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(jumpFloodTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(targetResolutionX, PostProcessingThreadGroupCountX);
                    uint32 groupCountY = ComputeWorkGroupCount(targetResolutionY, PostProcessingThreadGroupCountY);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(jumpFloodTexture)));
                    shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(outputTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::EditorSelectionOutlineComposite);
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