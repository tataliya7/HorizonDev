#include "PostProcessingPipeline.h"

namespace Horizon
{
    RenderGraphTextureHandle PostProcessingPipeline::DispatchFinalComposition(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle colorTexture,
        RenderGraphTextureHandle bloomTexture,
        RenderGraphTextureHandle localToneMappingTexture,
        RenderGraphTextureHandle colorTransformLUTTexture,
        RenderGraphBufferHandle autoExposureBuffer)
    {
        assert(colorTexture);
        assert(colorTransformLUTTexture);
        assert(autoExposureBuffer);

        const bool isBloomTextureValid = !bloomTexture.IsNull();
        const bool isLocalToneMappingTextureValid = !localToneMappingTexture.IsNull();

        if (!isBloomTextureValid)
        {
            bloomTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        }

        RenderBackendTextureHandle lensDirtTexture = defaultResources->GetBlackDummyTexture2D()->GetHandle();

        Vector2f chromaticAberrationScale = Vector2f(0.0f, 0.0f);

        RenderBackendTextureFormat outputTextureFormat = RenderBackendTextureFormat::R10G10B10A2Unorm;
        RenderGraphTextureDescription outputTextureDesc = RenderGraphTextureDescription::Create2D(
            targetResolution.width,
            targetResolution.height,
            outputTextureFormat,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);

        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");

        renderGraph.AddPass(
            std::format("FinalComposition (Compute, {}x{})", outputTextureDesc.width, outputTextureDesc.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                colorTexture = builder.ReadTexture(colorTexture, RenderBackendResourceState::ShaderResource);
                bloomTexture = builder.ReadTexture(bloomTexture, RenderBackendResourceState::ShaderResource);
                localToneMappingTexture = builder.ReadTexture(localToneMappingTexture, RenderBackendResourceState::ShaderResource);
                colorTransformLUTTexture = builder.ReadTexture(colorTransformLUTTexture, RenderBackendResourceState::ShaderResource);
                autoExposureBuffer = builder.ReadBuffer(autoExposureBuffer, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(outputTextureDesc.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(outputTextureDesc.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(colorTexture));
                    pushConstantValues.BindTextureSRV(2, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(lensDirtTexture));
                    pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(bloomTexture));
                    pushConstantValues.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(localToneMappingTexture));
                    pushConstantValues.BindTextureSRV(5, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(colorTransformLUTTexture));
                    pushConstantValues.BindBufferSRV(6, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(autoExposureBuffer));
                    pushConstantValues.BindTextureUAV(7, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::FinalComposition);

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
