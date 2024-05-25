#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::AddToneMappingPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        RenderGraphTextureHandle colorLUTTexture,
        RenderGraphTextureHandle bloomTexture,
        RenderGraphTextureHandle localExposureTexture,
        RenderGraphBufferHandle autoExposureBuffer,
        bool outputInHDR)
    {
        assert(sceneColorTexture);
        assert(colorLUTTexture);
        assert(is);

        const bool isLocalExposureTextureValid = !localExposureTexture.IsNullHandle();
        const bool isAutoExposureTextureValid = !autoExposureBuffer.IsNullHandle();
        const bool isBloomTextureValid = !bloomTexture.IsNullHandle();

        if (!isAutoExposureTextureValid)
        {
            const float defaultExposure = 1.0f; // TODO: Get fixed exposure
            // autoExposureTexture = renderGraph.CreateTexture(RenderGraphTextureDesc::Create2D(1, 1, RenderBackendTextureFormat::R32Float, RenderBackendTextureCreateFlags::ShaderResource), "AutoExposureTexture");
        }

        if (!isBloomTextureValid)
        {
            bloomTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
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

        RenderBackendTextureFormat outputFormat = RenderBackendTextureFormat::RGB10A2Unorm;

        RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
            targetResolution.width,
            targetResolution.height,
            outputFormat,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "ToneMappingTexture");

        renderGraph.AddPass(std::format("ToneMapping (Compute, {}x{})", targetResolution.width, targetResolution.height), RenderGraphPassFlags::Compute,
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
                        uint32 groupCountX = ComputeWorkGroupCount(targetResolution.width, PostProcessingThreadGroupCountX);
                        uint32 groupCountY = ComputeWorkGroupCount(targetResolution.height, PostProcessingThreadGroupCountY);
                        uint32 groupCountZ = 1;

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorTexture)));
                        shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(bloomTexture)));
                        shaderArguments.BindBuffer(3, registry.GetRenderBackendBufferHandle(autoExposureBuffer));
                        shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(colorLUTTexture)));
                        shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(lensDirtTexture));
                        if (isLocalExposureTextureValid) shaderArguments.BindTextureSRV(6, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureTexture)));
                        //else shaderArguments.BindTextureSRV(6, RenderBackendTextureSRVDesc::Create(RenderBackendTextureHandle::Null));
                        shaderArguments.BindTextureSRV(10, RenderBackendTextureSRVDesc::Create(testTexture));
                        shaderArguments.BindTextureUAV(7, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(outputTexture), 0));

                        shaderArguments.PushConstants(0, (float)flags);
                        shaderArguments.PushConstants(1, chromaticAberrationScale.x);
                        shaderArguments.PushConstants(2, chromaticAberrationScale.y);

                        RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::ToneMapping);

                        commandList.Dispatch(
                            computeShader,
                            shaderArguments,
                            groupCountX,
                            groupCountY,
                            groupCountZ);
                    };
            });

        return outputTexture;
    }
}