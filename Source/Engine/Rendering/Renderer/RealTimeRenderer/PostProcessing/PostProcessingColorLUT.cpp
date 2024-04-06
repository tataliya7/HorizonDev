#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace HE
{
    RenderGraphTextureHandle RealTimeRenderer::AddColorLUTPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        static const uint32 colorLUTTextureSize = 32;

        static const uint32 dispatchX = colorLUTTextureSize / 8;
        static const uint32 dispatchY = colorLUTTextureSize / 8;
        static const uint32 dispatchZ = colorLUTTextureSize / 8;

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
                        shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                        shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(colorLUTTexture), 0));
                        shaderArguments.PushConstants(0, (float)toneMappingOperator);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::ColorLUT);
                        commandList.Dispatch(
                            computeShader,
                            shaderArguments,
                            dispatchX,
                            dispatchY,
                            dispatchZ);
                    };
            });

        return colorLUTTexture;
    }
}