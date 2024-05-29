#include "../RealTimeRenderer.h"
#include "PostProcessingCommon.h"

namespace Horizon
{
    static constexpr uint32 GColorLUTTextureSize = 32;

    RenderGraphTextureHandle RealTimeRenderer::AddColorLUTPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        constexpr uint32 threadGroupCountX = GColorLUTTextureSize / 8;
        constexpr uint32 threadGroupCountY = GColorLUTTextureSize / 8;
        constexpr uint32 threadGroupCountZ = GColorLUTTextureSize / 8;

        RenderGraphTextureDesc colorLUTTextureDesc = RenderGraphTextureDesc::Create3D(
            GColorLUTTextureSize,
            GColorLUTTextureSize,
            GColorLUTTextureSize,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle colorLUTTexture = renderGraph.CreateTexture(colorLUTTextureDesc, "colorLUTTexture");

        //ToneMappingOperatorType toneMappingOperator = settings.toneMappingOperator;

        //if (toneMappingOperator == ToneMappingOperatorType::Hable)
        {
            // Hable Filmic Tone Curve
        }

        renderGraph.AddPass(
            std::format("ColorLUT (Compute, {}x{}x{})", GColorLUTTextureSize, GColorLUTTextureSize, GColorLUTTextureSize),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                colorLUTTexture = builder.WriteTexture(colorLUTTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(colorLUTTexture), 0));
                    //shaderArguments.PushConstants(0, (float)toneMappingOperator);

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