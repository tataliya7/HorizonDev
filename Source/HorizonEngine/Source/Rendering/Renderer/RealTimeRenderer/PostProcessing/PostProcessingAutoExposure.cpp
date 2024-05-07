#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::AddAutoExposureBuildHistogramPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture)
    {
        // TODO: Add it to shaders
        const static uint32 histogramBinCount = 256;

        const RenderGraphTextureDesc& inputTextureDesc = renderGraph.GetTextureDesc(sceneColorTexture);

        RenderGraphTextureDesc histogramTextureDesc = RenderGraphTextureDesc::Create1D(
            histogramBinCount,
            RenderBackendTextureFormat::R32Uint,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle histogramTexture = renderGraph.CreateTexture(histogramTextureDesc, "AutoExposureHistogramTexture");

        renderGraph.AddPass(std::format("AutoExposureBuildHistogram (Compute, {}x{})", inputTextureDesc.width, inputTextureDesc.height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);

                histogramTexture = builder.WriteTexture(histogramTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    // Quarter resolution
                    uint32 groupCountX = ComputeWorkGroupCount(inputTextureDesc.width, 16);
                    uint32 groupCountY = ComputeWorkGroupCount(inputTextureDesc.height, 16);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer(), 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorTexture)));
                    shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(histogramTexture), 0));

                    commandList.ClearTextureUAV(RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(histogramTexture), 0), RenderBackendTextureClearValue::CreateColorValueFloat4(0.0f, 0.0f, 0.0f, 0.0f));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::AutoExposureBuildHistogram);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
                };
            });

        return histogramTexture;
    }

    RenderGraphBufferHandle RealTimeRenderer::AddAutoExposureComputeExposurePass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle autoExposureHistogramTexture,
        RenderGraphBufferHandle previousAutoExposureBuffer)
    {
        RenderGraphBufferDesc autoExposureBufferDesc = RenderGraphBufferDesc::CreateByteAddress(sizeof(AutoExposureData));
        RenderGraphBufferHandle autoExposureBuffer = renderGraph.CreateBuffer(autoExposureBufferDesc, "AutoExposureBuffer");

        renderGraph.AddPass(std::format("AutoExposureComputeExposure (Compute, 1x1x1)"), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                autoExposureHistogramTexture = builder.ReadTexture(autoExposureHistogramTexture, RenderBackendResourceState::ShaderResource);
                previousAutoExposureBuffer = builder.ReadBuffer(previousAutoExposureBuffer, RenderBackendResourceState::ShaderResource);

                autoExposureBuffer = builder.WriteBuffer(autoExposureBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer(), 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(autoExposureHistogramTexture)));
                    shaderArguments.BindBuffer(2, registry.GetRenderBackendBufferHandle(previousAutoExposureBuffer), 0);
                    shaderArguments.BindBuffer(3, registry.GetRenderBackendBufferHandle(autoExposureBuffer), 0);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::AutoExposureComputeExposure);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        1, 1, 1);
                };
            });

        currentAutoExposureReadbackBufferIndex = (currentAutoExposureReadbackBufferIndex + 1) % NumAutoExposureReadbackBuffers;
        RenderGraphBufferHandle autoExposureReadbackBuffer = renderGraph.ImportExternalBuffer(autoExposureReadbackBuffers[currentAutoExposureReadbackBufferIndex]);

        renderGraph.AddPass(std::format("AutoExposureBufferReadback (Copy, {})"), RenderGraphPassFlags::Readback,
            [&](RenderGraphBuilder& builder)
            {
                autoExposureBuffer = builder.ReadBuffer(autoExposureBuffer, RenderBackendResourceState::CopySrc);
                autoExposureReadbackBuffer = builder.WriteBuffer(autoExposureReadbackBuffer, RenderBackendResourceState::CopyDst);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.CopyBuffer(registry.GetRenderBackendBufferHandle(autoExposureBuffer), 0, registry.GetRenderBackendBufferHandle(autoExposureReadbackBuffer), 0, sizeof(AutoExposureData));
                };
            });

        renderGraph.ExportBufferDeferred(autoExposureBuffer, autoExposureBufferHistory);

        return autoExposureBuffer;
    }
}