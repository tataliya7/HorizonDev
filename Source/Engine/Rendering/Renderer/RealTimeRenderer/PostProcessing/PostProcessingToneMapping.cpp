#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace HE
{
    RenderGraphTextureHandle RealTimeRenderer::AddToneMappingPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        RenderGraphTextureHandle bloomTexture,
        RenderGraphBufferHandle autoExposureBuffer,
        RenderGraphTextureHandle colorLUTTexture,
        RenderGraphTextureHandle localExposureTexture,
        bool outputInHDR)
    {
        const bool isAutoExposureTextureValid = !autoExposureBuffer.IsNullHandle();
        if (!isAutoExposureTextureValid)
        {
            const float defaultExposure = 1.4f; // TODO: Get fixed exposure
            // autoExposureTexture = renderGraph.CreateTexture(RenderGraphTextureDesc::Create2D(1, 1, RenderBackendTextureFormat::R32Float, RenderBackendTextureCreateFlags::ShaderResource), "AutoExposureTexture");
        }

        const bool isLocalExposureTextureValid = !localExposureTexture.IsNullHandle();

        if (!bloomTexture)
        {
            bloomTexture = renderGraph.ImportExternalTexture(blackDummyTexture, dummyTextureDesc, RenderBackendResourceState::ShaderResource, "BlackDummyTexture");
        }

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

        //RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
        //      targetResolutionX,
        //      targetResolutionY,
        //      RenderBackendTextureFormat::RGBA8Unorm,
        //      RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        auto& finalTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();
        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(finalTextureData.finalTextureDesc, "ToneMappingTexture");

        renderGraph.AddPass(std::format("ToneMapping (Compute, {}x{})", targetResolutionX, targetResolutionY), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(bloomTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadBuffer(autoExposureBuffer, RenderBackendResourceState::UnorderedAccess);
                builder.ReadTexture(colorLUTTexture, RenderBackendResourceState::ShaderResource);
                //builder.ReadTexture(lensDirtTexture, RenderBackendResourceState::ShaderResource);
                if (isLocalExposureTextureValid) builder.ReadTexture(localExposureTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 dispatchX = Math::CeilDiv(targetResolutionX, PostProcessingThreadGroupCountX);
                    uint32 dispatchY = Math::CeilDiv(targetResolutionY, PostProcessingThreadGroupCountY);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneColorTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(bloomTexture)));
                    shaderArguments.BindBuffer(3, registry.GetRenderBackendBuffer(autoExposureBuffer), 0);
                    shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(colorLUTTexture)));
                    shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(lensDirtTexture));
                    if (isLocalExposureTextureValid) shaderArguments.BindTextureSRV(6, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(localExposureTexture)));
                    //else shaderArguments.BindTextureSRV(6, RenderBackendTextureSRVDesc::Create(RenderBackendTextureHandle::Null));
                    shaderArguments.BindTextureUAV(7, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(outputTexture), 0));
                    shaderArguments.PushConstants(0, (float)flags);

                    shaderArguments.PushConstants(1, chromaticAberrationScale.x);
                    shaderArguments.PushConstants(2, chromaticAberrationScale.y);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::ToneMapping);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        dispatchX,
                        dispatchY);
                };
            });

        return outputTexture;
    }
}