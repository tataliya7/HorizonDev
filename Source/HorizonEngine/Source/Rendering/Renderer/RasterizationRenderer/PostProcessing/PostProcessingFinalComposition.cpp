#include "../RasterizationRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    RenderGraphTextureHandle RasterizationRenderer::DispatchFinalComposition(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle colorTexture,
        RenderGraphTextureHandle bloomTexture,
        RenderGraphTextureHandle localToneMappingTexture,
        RenderGraphTextureHandle colorTransformLUTTexture,
        RenderGraphBufferHandle autoExposureBuffer,
        bool outputInHDR)
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

        // if (!isLocalToneMappingTextureValid)
        // {
        //     localToneMappingTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        // }
        // TODO: Implement lens dirt
        RenderBackendTextureHandle lensDirtTexture = defaultResources->GetBlackDummyTexture2D()->GetHandle();

        uint32 flags = 0;

        // Chromatic Aberration
        Vector2f chromaticAberrationScale = Vector2f(0.0f, 0.0f);
        // TODO: Implement chromatic aberration
        // {
        //     float chromaticAberrationIntensity = settings.postProcessingSettings.chromaticAberrationIntensity;
        //     float chromaticAberrationOffset = settings.postProcessingSettings.chromaticAberrationOffset;
        //
        //     // Wavelength of primarie colors in nm
        //     const float wavelengthR = 611.3f;
        //     const float wavelengthG = 549.1f;
        //     const float wavelengthB = 464.3f;
        //
        //     const float scaleR = 0.007f * (wavelengthR - wavelengthB);
        //     const float scaleG = 0.007f * (wavelengthG - wavelengthB);
        //
        //     if (chromaticAberrationOffset < 1.0f)
        //     {
        //         float offset = chromaticAberrationIntensity * 0.01f;
        //         float multiplier = 1.0f / (1.0f - chromaticAberrationOffset);
        //         chromaticAberrationScale = Vector2f(scaleR * offset * multiplier, scaleG * offset * multiplier);
        //     }
        // }

        RenderBackendTextureFormat outputTextureFormat = RenderBackendTextureFormat::R10G10B10A2Unorm;
        RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolution.width,
            targetResolution.height,
            outputTextureFormat,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);

        // TODO
        //RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "ToneMappingTexture");
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

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(colorTexture));
                    shaderConstants.BindTextureSRV(2, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(lensDirtTexture));
                    shaderConstants.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(bloomTexture));
                    shaderConstants.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(localToneMappingTexture));
                    shaderConstants.BindTextureSRV(5, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(colorTransformLUTTexture));
                    shaderConstants.BindBufferSRV(6, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(autoExposureBuffer));
                    shaderConstants.BindTextureUAV(7, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    //shaderConstants.BindTextureSRV(10, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(testTexture));

                    //shaderConstants.BindScalar(0, (float)flags);
                    //shaderConstants.BindScalar(1, chromaticAberrationScale.x);
                    //shaderConstants.BindScalar(2, chromaticAberrationScale.y);

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::FinalComposition);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }
}