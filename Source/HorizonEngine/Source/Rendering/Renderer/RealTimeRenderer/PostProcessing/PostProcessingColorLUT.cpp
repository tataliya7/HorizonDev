#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::AddColorLUTPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        static const uint32 colorLUTTextureSize = 32;

        static const uint32 groupCountX = colorLUTTextureSize / 8;
        static const uint32 groupCountY = colorLUTTextureSize / 8;
        static const uint32 groupCountZ = colorLUTTextureSize / 8;

        RenderGraphTextureDesc colorLUTTextureDesc = RenderGraphTextureDesc::Create3D(
            colorLUTTextureSize, colorLUTTextureSize, colorLUTTextureSize,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle colorLUTTexture = renderGraph.CreateTexture(colorLUTTextureDesc, "colorLUTTexture");

        ToneMappingOperatorType toneMappingOperator = settings.toneMappingOperator;

        if (toneMappingOperator == ToneMappingOperatorType::Hable)
        {
            // Hable Filmic Tone Curve
        }

        renderGraph.AddPass(std::format("ColorLUT (Compute, {}x{}x{})", colorLUTTextureSize, colorLUTTextureSize, colorLUTTextureSize), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                colorLUTTexture = builder.WriteTexture(colorLUTTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(colorLUTTexture), 0));
                        shaderArguments.PushConstants(0, (float)toneMappingOperator);

                        RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::ColorLUT);
                        commandList.Dispatch(
                            computeShader,
                            shaderArguments,
                            groupCountX,
                            groupCountY,
                            groupCountZ);
                    };
            });

        return colorLUTTexture;
    }
}