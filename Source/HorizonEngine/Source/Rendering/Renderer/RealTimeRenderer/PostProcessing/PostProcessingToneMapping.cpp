#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::AddToneMappingPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        RenderGraphTextureHandle bloomTexture,
        RenderGraphTextureHandle localExposureTexture,
        RenderGraphTextureHandle colorLUTTexture,
        RenderGraphBufferHandle autoExposureBuffer,
        bool outputInHDR)
    {
        assert(sceneColorTexture);
        assert(colorLUTTexture);
        assert(autoExposureBuffer);

        const bool isBloomTextureValid = !bloomTexture.IsNullHandle();
        const bool isLocalExposureTextureValid = !localExposureTexture.IsNullHandle();

        if (!isBloomTextureValid)
        {
            bloomTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        }

        if (!isLocalExposureTextureValid)
        {
            localExposureTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        }
        // TODO: Implement lens dirt
        RenderBackendTextureHandle lensDirtTexture = defaultResources->GetBlackDummyTexture2D()->GetHandle();

        uint32 flags = 0;

        // Chromatic Aberration
        Vector2 chromaticAberrationScale = Vector2(0.0f, 0.0f);
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
        //         chromaticAberrationScale = Vector2(scaleR * offset * multiplier, scaleG * offset * multiplier);
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
            std::format("ToneMapping (Compute, {}x{})", outputTextureDesc.width, outputTextureDesc.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                sceneColorTexture = builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);
                bloomTexture = builder.ReadTexture(bloomTexture, RenderBackendResourceState::ShaderResource);
                localExposureTexture = builder.ReadTexture(localExposureTexture, RenderBackendResourceState::ShaderResource);
                colorLUTTexture = builder.ReadTexture(colorLUTTexture, RenderBackendResourceState::ShaderResource);
                autoExposureBuffer = builder.ReadBuffer(autoExposureBuffer, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = CeilDiv(outputTextureDesc.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = CeilDiv(outputTextureDesc.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneColorTexture));
                    shaderConstants.BindTextureSRV(2, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(lensDirtTexture));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(bloomTexture));
                    shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(localExposureTexture));
                    shaderConstants.BindTextureSRV(5, registry.GetTextureSRVBindlessResourceDescriptorIndex(colorLUTTexture));
                    shaderConstants.BindBufferSRV(6, registry.GetBufferSRVBindlessResourceDescriptorIndex(autoExposureBuffer));
                    shaderConstants.BindTextureUAV(7, registry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    //shaderConstants.BindTextureSRV(10, registry.GetTextureSRVBindlessResourceDescriptorIndex(testTexture));

                    //shaderConstants.BindScalar(0, (float)flags);
                    //shaderConstants.BindScalar(1, chromaticAberrationScale.x);
                    //shaderConstants.BindScalar(2, chromaticAberrationScale.y);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::ToneMapping);

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