#include "../RasterizationRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    // TODO: Make it configurable
    static constexpr uint32 AutoExposureHistogramBinCount = 256;

    void RasterizationRenderer::UpdateAutoExposureDataFromReadbackBuffer()
    {
        RenderGraphPersistentBuffer* autoExposureReadbackBuffer = autoExposureReadbackBuffers[currentAutoExposureReadbackBufferIndex];
        if (autoExposureReadbackBuffer != nullptr)
        {
            void* data = nullptr;
            renderBackend->MapBuffer(autoExposureReadbackBuffer->GetHandle(), &data);
            if (data != nullptr)
            {
                autoExposureData.adaptedExposure = static_cast<float*>(data)[0];
                autoExposureData.targetExposure = static_cast<float*>(data)[1];
                autoExposureData.exposureCompensation = static_cast<float*>(data)[2];
                autoExposureData.averageSceneLuminance = static_cast<float*>(data)[3];
                renderBackend->UnmapBuffer(autoExposureReadbackBuffer->GetHandle());
            }
        }
    }

    RenderGraphBufferHandle RasterizationRenderer::DispatchHistogramBasedAutoExposure(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle colorTexture,
        const PostProcessingColorPyramid& colorPyramid,
        RenderGraphBufferHandle previousAutoExposureBuffer)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "AutoExposure");

        RenderGraphTextureHandle inputColorTexture = colorPyramid.textures[0];
        const RenderGraphTextureDescription& inputColorTextureDescription = renderGraph.GetTextureDesc(inputColorTexture);

        RenderGraphTextureDescription autoExposureHistogramTextureDescription = RenderGraphTextureDescription::Create1D(
            AutoExposureHistogramBinCount,
            RenderBackendTextureFormat::R32Uint,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle autoExposureHistogramTexture = renderGraph.CreateTexture(autoExposureHistogramTextureDescription, "AutoExposureHistogramTexture");

        renderGraph.AddPass(
            std::format("AutoExposureBuildHistogram (Compute, {}x{})", inputColorTextureDescription.width, inputColorTextureDescription.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, inputColorTexture);
                builder.SetBindlessResourceUAV(2, autoExposureHistogramTexture, 0);

                RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::AutoExposureBuildHistogram);

                uint32 threadGroupCountX = ComputeShaderThreadGroupCount(inputColorTextureDescription.width, 16);
                uint32 threadGroupCountY = ComputeShaderThreadGroupCount(inputColorTextureDescription.height, 16);
                uint32 threadGroupCountZ = 1;

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.ClearTextureUAV(RenderBackendTextureUAVDesc::Create(resourceRegistry.GetRenderBackendTextureHandle(autoExposureHistogramTexture), 0), RenderBackendTextureClearValue::CreateColorValueFloat4(0.0f, 0.0f, 0.0f, 0.0f));

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        RenderGraphBufferDescription autoExposureBufferDescription = RenderGraphBufferDescription::CreateByteAddress(sizeof(AutoExposureData));
        RenderGraphBufferHandle autoExposureBuffer = renderGraph.CreateBuffer(autoExposureBufferDescription, "AutoExposureBuffer");

        renderGraph.AddPass(
            std::format("AutoExposureComputeExposure (Compute, 1x1x1)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, autoExposureHistogramTexture);
                builder.SetBindlessResourceSRV(2, previousAutoExposureBuffer);
                builder.SetBindlessResourceUAV(3, autoExposureBuffer);

                RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::AutoExposureComputeExposure);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        1,
                        1,
                        1);
                };
            });

        currentAutoExposureReadbackBufferIndex = (currentAutoExposureReadbackBufferIndex + 1) % AutoExposureReadbackBufferCount;

        RenderGraphBufferHandle autoExposureReadbackBuffer = renderGraph.ImportExternalBuffer(autoExposureReadbackBuffers[currentAutoExposureReadbackBufferIndex], "AutoExposureReadbackBuffer");

        renderGraph.AddPass(
            std::format("AutoExposureBufferReadback (Copy, {} bytes)", sizeof(AutoExposureData)),
            RenderGraphPassFlags::Readback,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadBuffer(autoExposureBuffer, RenderBackendResourceState::CopySrc);
                builder.WriteBuffer(autoExposureReadbackBuffer, RenderBackendResourceState::CopyDst);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.CopyBuffer(
                        resourceRegistry.GetRenderBackendBufferHandle(autoExposureBuffer),
                        0,
                        resourceRegistry.GetRenderBackendBufferHandle(autoExposureReadbackBuffer),
                        0,
                        sizeof(AutoExposureData));
                };
            });

        renderGraph.ExportBufferDeferred(autoExposureBuffer, &historyFrame.autoExposureBuffer);

        return autoExposureBuffer;
    }

    RenderGraphTextureHandle RasterizationRenderer::AddCopyExposurePass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphBufferHandle autoExposureBuffer)
    {
        RenderGraphTextureDescription exposureTextureDesc = RenderGraphTextureDescription::Create2D(
            1,
            1,
            RenderBackendTextureFormat::R32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle exposureTexture = renderGraph.CreateTexture(exposureTextureDesc, "ExposureTexture");

        renderGraph.AddPass(
            std::format("CopyExposure (Compute, {}x{})", 1, 1),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, autoExposureBuffer);
                builder.SetBindlessResourceUAV(1, exposureTexture, 0);

                RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::CopyExposure);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        1,
                        1,
                        1);
                };
            });

        return exposureTexture;
    }
}