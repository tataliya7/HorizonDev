#include "../RasterizationRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    // TODO: Make it configurable
    static constexpr uint32 GHistogramBinCount = 256;

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

        RenderGraphTextureDescription autoExposureHistogramTextureDesc = RenderGraphTextureDescription::Create1D(
            GHistogramBinCount,
            RenderBackendTextureFormat::R32Uint,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle autoExposureHistogramTexture = renderGraph.CreateTexture(autoExposureHistogramTextureDesc, "AutoExposureHistogramTexture");

        renderGraph.AddPass(
            std::format("AutoExposureBuildHistogram (Compute, {}x{})", inputColorTextureDescription.width, inputColorTextureDescription.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                inputColorTexture = builder.ReadTexture(inputColorTexture, RenderBackendResourceState::ShaderResource);
                autoExposureHistogramTexture = builder.WriteTexture(autoExposureHistogramTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(inputColorTextureDescription.width, 16);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(inputColorTextureDescription.height, 16);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(inputColorTexture));
                    shaderConstants.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(autoExposureHistogramTexture, 0));

                    commandList.ClearTextureUAV(RenderBackendTextureUAVDesc::Create(resourceRegistry.GetRenderBackendTextureHandle(autoExposureHistogramTexture), 0), RenderBackendTextureClearValue::CreateColorValueFloat4(0.0f, 0.0f, 0.0f, 0.0f));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::AutoExposureBuildHistogram);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        RenderGraphBufferDesc autoExposureBufferDesc = RenderGraphBufferDesc::CreateByteAddress(sizeof(AutoExposureData));
        RenderGraphBufferHandle autoExposureBuffer = renderGraph.CreateBuffer(autoExposureBufferDesc, "AutoExposureBuffer");

        renderGraph.AddPass(
            std::format("AutoExposureComputeExposure (Compute, 1x1x1)"),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                autoExposureHistogramTexture = builder.ReadTexture(autoExposureHistogramTexture, RenderBackendResourceState::ShaderResource);
                previousAutoExposureBuffer = builder.ReadBuffer(previousAutoExposureBuffer, RenderBackendResourceState::ShaderResource);
                autoExposureBuffer = builder.WriteBuffer(autoExposureBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(autoExposureHistogramTexture));
                    shaderConstants.BindBufferSRV(2, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(previousAutoExposureBuffer));
                    shaderConstants.BindBufferUAV(3, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(autoExposureBuffer));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::AutoExposureComputeExposure);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        1,
                        1,
                        1);
                };
            });

        currentAutoExposureReadbackBufferIndex = (currentAutoExposureReadbackBufferIndex + 1) % NumAutoExposureReadbackBuffers;

        RenderGraphBufferHandle autoExposureReadbackBuffer = renderGraph.ImportExternalBuffer(autoExposureReadbackBuffers[currentAutoExposureReadbackBufferIndex], "AutoExposureReadbackBuffer");

        renderGraph.AddPass(
            std::format("AutoExposureBufferReadback (Copy, {} bytes)", sizeof(AutoExposureData)),
            RenderGraphPassFlags::Readback,
            [&](RenderGraphBuilder& builder)
            {
                autoExposureBuffer = builder.ReadBuffer(autoExposureBuffer, RenderBackendResourceState::CopySrc);
                autoExposureReadbackBuffer = builder.WriteBuffer(autoExposureReadbackBuffer, RenderBackendResourceState::CopyDst);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.CopyBuffer(resourceRegistry.GetRenderBackendBufferHandle(autoExposureBuffer), 0, resourceRegistry.GetRenderBackendBufferHandle(autoExposureReadbackBuffer), 0, sizeof(AutoExposureData));
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
                autoExposureBuffer = builder.ReadBuffer(autoExposureBuffer, RenderBackendResourceState::ShaderResource);
                exposureTexture = builder.WriteTexture(exposureTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(autoExposureBuffer));
                    shaderConstants.BindTextureUAV(1, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(exposureTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::CopyExposure);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        1,
                        1,
                        1);
                };
            });

        return exposureTexture;
    }
}