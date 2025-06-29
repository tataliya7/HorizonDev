#include "../RasterizationRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    static constexpr uint32 ColorTransformLUTTextureSize = 32;

    static constexpr uint32 threadGroupCountX = ColorTransformLUTTextureSize / 8;
    static constexpr uint32 threadGroupCountY = ColorTransformLUTTextureSize / 8;
    static constexpr uint32 threadGroupCountZ = ColorTransformLUTTextureSize / 8;

    bool PostProcessingColorTransformLUTSettings::Update(const SceneView& view, const RasterizationRendererPostProcessingSettings& postProcessingSettings)
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

    RenderGraphTextureHandle RasterizationRenderer::RenderColorTransformLUT(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const bool shouldUpdateColorTransformLUT = cachedColorTransformLUTSettings.Update(view, finalPostProcessingSettings);
        const bool forceUpdateColorTransformLUT = true;

        if (!shouldUpdateColorTransformLUT && !forceUpdateColorTransformLUT)
        {
            RenderGraphTextureHandle colorTransformLUTTexture = renderGraph.ImportExternalTexture(cachedColorTransformLUTTexture, "ColorTransformLUTTexture");
            return colorTransformLUTTexture;
        }

        RenderGraphTextureDescription colorTransformLUTTextureDesc = RenderGraphTextureDescription::Create3D(
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

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(colorTransformLUTTexture, 0));
                    //pushConstantValues.PushConstants(0, (float)toneMappingOperator);

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::ColorTransformLUT);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.ExportTextureDeferred(colorTransformLUTTexture, &cachedColorTransformLUTTexture);

        return colorTransformLUTTexture;
    }
}