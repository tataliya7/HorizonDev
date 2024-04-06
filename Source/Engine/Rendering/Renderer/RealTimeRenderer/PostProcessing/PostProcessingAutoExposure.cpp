#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace HE
{
    RenderGraphTextureHandle RealTimeRenderer::AddAutoExposureBuildHistogramPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture)
    {
        RenderGraphTextureDesc histogramTextureDesc = RenderGraphTextureDesc::Create2D(
            256,
            1,
            RenderBackendTextureFormat::R32Uint,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle histogramTexture = renderGraph.CreateTexture(histogramTextureDesc, "AutoExposureHistogramTexture");

        uint32 width = targetResolutionX;
        uint32 height = targetResolutionY;

        renderGraph.AddPass(std::format("AutoExposureBuildHistogram (Compute, {}, {})", width, height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);

                histogramTexture = builder.WriteTexture(histogramTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    // Quarter resolution
                    uint32 dispatchX = Math::CeilDiv(width, 16);
                    uint32 dispatchY = Math::CeilDiv(height, 16);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneColorTexture)));
                    shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(histogramTexture), 0));

                    commandList.ClearTextureUAV(RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(histogramTexture), 0), RenderBackendTextureClearValue::CreateColorValueFloat4(0.0f, 0.0f, 0.0f, 0.0f));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::AutoExposureBuildHistogram);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        dispatchX,
                        dispatchY);
                };
            });
        return histogramTexture;
    }

    RenderGraphBufferHandle RealTimeRenderer::AddAutoExposureComputeExposurePass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle autoExposureHistogramTexture,
        RenderGraphBufferHandle historyAutoExposureBuffer)
    {
        RenderGraphBufferDesc autoExposureBufferDesc = RenderGraphBufferDesc::CreateByteAddress(16);
        RenderGraphBufferHandle autoExposureBuffer = renderGraph.CreateBuffer(autoExposureBufferDesc, "AutoExposureBuffer");

        renderGraph.AddPass("AutoExposureComputeExposure (Compute)", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                autoExposureHistogramTexture = builder.ReadTexture(autoExposureHistogramTexture, RenderBackendResourceState::ShaderResource);
                historyAutoExposureBuffer = builder.ReadBuffer(historyAutoExposureBuffer, RenderBackendResourceState::ShaderResource);

                autoExposureBuffer = builder.WriteBuffer(autoExposureBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(autoExposureHistogramTexture)));
                    shaderArguments.BindBuffer(2, registry.GetRenderBackendBuffer(historyAutoExposureBuffer), 0);
                    shaderArguments.BindBuffer(3, registry.GetRenderBackendBuffer(autoExposureBuffer), 0);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::AutoExposureComputeExposure);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        1,
                        1,
                        1);
                };
            });

        currentAutoExposureReadbackBufferIndex = (currentAutoExposureReadbackBufferIndex + 1) % NumAutoExposureReadbackBuffers;
        RenderGraphBufferHandle autoExposureReadbackBuffer = renderGraph.ImportExternalBuffer(&autoExposureReadbackBuffers[currentAutoExposureReadbackBufferIndex]);

        renderGraph.AddPass("AutoExposureBufferReadback (Copy)", RenderGraphPassFlags::Readback,
            [&](RenderGraphBuilder& builder)
            {
                autoExposureBuffer = builder.ReadBuffer(autoExposureBuffer, RenderBackendResourceState::CopySrc);
                autoExposureReadbackBuffer = builder.WriteBuffer(autoExposureReadbackBuffer, RenderBackendResourceState::CopyDst);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.CopyBuffer(registry.GetRenderBackendBuffer(autoExposureBuffer), 0, registry.GetRenderBackendBuffer(autoExposureReadbackBuffer), 0, 4);
                };
            });

        renderGraph.ExportBufferDeferred(autoExposureBuffer, &historyAutoExposureBufferPersistent);

        return autoExposureBuffer;
    }
}