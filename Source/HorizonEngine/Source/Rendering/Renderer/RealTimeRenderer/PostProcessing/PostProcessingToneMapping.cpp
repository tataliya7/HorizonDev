#include "../RealTimeRenderer.h"
#include "PostProcessingCommon.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::AddToneMappingPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        RenderGraphTextureHandle bloomTexture,
        RenderGraphTextureHandle colorLUTTexture,
        RenderGraphTextureHandle localExposureTexture,
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

        // TODO: Implement lens dirt
        RenderBackendTextureHandle lensDirtTexture = defaultResources->blackDummyTexture2D->GetHandle();

        uint32 flags = 0;

        // Chromatic Aberration
        Vector2 chromaticAberrationScale = Vector2(0.0f, 0.0f);
        {
            float chromaticAberrationIntensity = settings.postProcessingSettings.chromaticAberrationIntensity;
            float chromaticAberrationOffset = settings.postProcessingSettings.chromaticAberrationOffset;

            // Wavelength of primarie colors in nm
            const float wavelengthR = 611.3f;
            const float wavelengthG = 549.1f;
            const float wavelengthB = 464.3f;

            const float scaleR = 0.007f * (wavelengthR - wavelengthB);
            const float scaleG = 0.007f * (wavelengthG - wavelengthB);

            if (chromaticAberrationOffset < 1.0f)
            {
                float offset = chromaticAberrationIntensity * 0.01f;
                float multiplier = 1.0f / (1.0f - chromaticAberrationOffset);
                chromaticAberrationScale = Vector2(scaleR * offset * multiplier, scaleG * offset * multiplier);
            }
        }

        RenderBackendTextureFormat outputTextureFormat = RenderBackendTextureFormat::RGB10A2Unorm;

        RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolution.width,
            targetResolution.height,
            outputTextureFormat,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "ToneMappingTexture");

        renderGraph.AddPass(
            std::format("ToneMapping (Compute, {}x{})", outputTextureDesc.width, outputTextureDesc.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                sceneColorTexture = builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);
                bloomTexture = builder.ReadTexture(bloomTexture, RenderBackendResourceState::ShaderResource);
                colorLUTTexture = builder.ReadTexture(colorLUTTexture, RenderBackendResourceState::ShaderResource);
                localExposureTexture = builder.ReadTexture(localExposureTexture, RenderBackendResourceState::ShaderResource);
                autoExposureBuffer = builder.ReadBuffer(autoExposureBuffer, RenderBackendResourceState::ShaderResource);
                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeWorkGroupCount(outputTextureDesc.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeWorkGroupCount(outputTextureDesc.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(lensDirtTexture));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorTexture)));
                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(bloomTexture)));
                    shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(colorLUTTexture)));
                    shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureTexture)));
                    shaderArguments.BindBuffer(6, registry.GetRenderBackendBufferHandle(autoExposureBuffer));
                    shaderArguments.BindTextureUAV(7, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(outputTexture), 0));

                    //shaderArguments.BindTextureSRV(10, RenderBackendTextureSRVDesc::Create(testTexture));

                    shaderArguments.PushConstants(0, (float)flags);
                    shaderArguments.PushConstants(1, chromaticAberrationScale.x);
                    shaderArguments.PushConstants(2, chromaticAberrationScale.y);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::ToneMapping);

                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }
}