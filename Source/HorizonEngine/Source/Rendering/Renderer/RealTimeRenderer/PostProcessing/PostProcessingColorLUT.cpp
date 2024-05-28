#include "../RealTimeRenderer.h"
#include "PostProcessingCommon.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::AddColorLUTPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        static const uint32 colorLUTTextureSize = 32;

        static const uint32 threadGroupCountX = colorLUTTextureSize / 8;
        static const uint32 threadGroupCountY = colorLUTTextureSize / 8;
        static const uint32 threadGroupCountZ = colorLUTTextureSize / 8;

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
                        shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(colorLUTTexture), 0));
                        shaderArguments.PushConstants(0, (float)toneMappingOperator);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::ColorLUT);
                        commandList.Dispatch(
                            computeShader,
                            shaderArguments,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    };
            });

        return colorLUTTexture;
    }
}