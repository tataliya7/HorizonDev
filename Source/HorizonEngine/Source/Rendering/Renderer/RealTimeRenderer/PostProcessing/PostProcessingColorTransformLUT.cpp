#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    static constexpr uint32 GColorTransformLUTTextureSize = 32;

    RenderGraphTextureHandle RealTimeRenderer::RenderColorTransformLUT(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        constexpr uint32 threadGroupCountX = GColorTransformLUTTextureSize / 8;
        constexpr uint32 threadGroupCountY = GColorTransformLUTTextureSize / 8;
        constexpr uint32 threadGroupCountZ = GColorTransformLUTTextureSize / 8;

        RenderGraphTextureDesc colorLUTTextureDesc = RenderGraphTextureDesc::Create3D(
            GColorTransformLUTTextureSize,
            GColorTransformLUTTextureSize,
            GColorTransformLUTTextureSize,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle colorLUTTexture = renderGraph.CreateTexture(colorLUTTextureDesc, "ColorTransformLUTTexture");

        //ToneMappingOperatorType toneMappingOperator = settings.toneMappingOperator;

        //if (toneMappingOperator == ToneMappingOperatorType::Hable)
        {
            // Hable Filmic Tone Curve
        }

        renderGraph.AddPass(
            std::format("ColorTransformLUT (Compute, {}x{}x{})", GColorTransformLUTTextureSize, GColorTransformLUTTextureSize, GColorTransformLUTTextureSize),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                colorLUTTexture = builder.WriteTexture(colorLUTTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureUAV(1, registry.GetTextureUAVBindlessResourceDescriptorIndex(colorLUTTexture, 0));
                    //shaderConstants.PushConstants(0, (float)toneMappingOperator);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::ColorTransformLUT);
                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return colorLUTTexture;
    }
}