#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    static constexpr uint32 ColorTransformLUTTextureSize = 32;

    static constexpr uint32 threadGroupCountX = ColorTransformLUTTextureSize / 8;
    static constexpr uint32 threadGroupCountY = ColorTransformLUTTextureSize / 8;
    static constexpr uint32 threadGroupCountZ = ColorTransformLUTTextureSize / 8;

    bool PostProcessingColorTransformLUTSettings::Update(const SceneView& view, const PostProcessingSettings& postProcessingSettings)
    {
        bool changed = false;

        if (!initialized)
        {
            whiteBalance = postProcessingSettings.whiteBalance;
            changed = true;
            initialized = true;
        }
        else
        {
            if (whiteBalance != postProcessingSettings.whiteBalance)
            {
                whiteBalance = postProcessingSettings.whiteBalance;
                changed = true;
            }
        }

        return changed;
    }

    RenderGraphTextureHandle RealTimeRenderer::RenderColorTransformLUT(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const bool shouldUpdateColorTransformLUT = cachedColorTransformLUTSettings.Update(view, finalPostProcessingSettings);
        const bool forceUpdateColorTransformLUT = false;

        if (!shouldUpdateColorTransformLUT && !forceUpdateColorTransformLUT)
        {
            RenderGraphTextureHandle colorTransformLUTTexture = renderGraph.ImportExternalTexture(cachedColorTransformLUTTexture, "ColorTransformLUTTexture");
            return colorTransformLUTTexture;
        }

        RenderGraphTextureDesc colorTransformLUTTextureDesc = RenderGraphTextureDesc::Create3D(
            ColorTransformLUTTextureSize,
            ColorTransformLUTTextureSize,
            ColorTransformLUTTextureSize,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle colorTransformLUTTexture = renderGraph.CreateTexture(colorTransformLUTTextureDesc, "ColorTransformLUTTexture");

        renderGraph.AddPass(
            std::format("ColorTransformLUT (Compute, {}x{}x{})", ColorTransformLUTTextureSize, ColorTransformLUTTextureSize, ColorTransformLUTTextureSize),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                colorTransformLUTTexture = builder.WriteTexture(colorTransformLUTTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureUAV(1, registry.GetTextureUAVBindlessResourceDescriptorIndex(colorTransformLUTTexture, 0));
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

        renderGraph.ExportTextureDeferred(colorTransformLUTTexture, &cachedColorTransformLUTTexture);

        return colorTransformLUTTexture;
    }
}