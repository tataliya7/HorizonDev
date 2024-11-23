#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    // TODO: Make it configurable
    static constexpr uint32 GHistogramBinCount = 256;

    bool RealTimeRenderer::IsAutoExposureEnabled() const
    {
        return features.enableAutoExposure;
    }

    void RealTimeRenderer::UpdateAutoExposureDataFromReadbackBuffer()
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

    RenderGraphBufferHandle RealTimeRenderer::DispatchHistogramBasedAutoExposure(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        RenderGraphBufferHandle previousAutoExposureBuffer)
    {
        const RenderGraphTextureDesc& sceneColorTextureDesc = renderGraph.GetTextureDesc(sceneColorTexture);

        RenderGraphTextureDesc autoExposureHistogramTextureDesc = RenderGraphTextureDesc::Create1D(
            GHistogramBinCount,
            RenderBackendTextureFormat::R32Uint,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle autoExposureHistogramTexture = renderGraph.CreateTexture(autoExposureHistogramTextureDesc, "AutoExposureHistogramTexture");

        renderGraph.AddPass(
            std::format("AutoExposureBuildHistogram (Compute, {}x{})", sceneColorTextureDesc.width, sceneColorTextureDesc.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                sceneColorTexture = builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);
                autoExposureHistogramTexture = builder.WriteTexture(autoExposureHistogramTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    // Quarter resolution
                    // TODO: change to 1/2 resolution
                    uint32 threadGroupCountX = CeilDiv(sceneColorTextureDesc.width, 16);
                    uint32 threadGroupCountY = CeilDiv(sceneColorTextureDesc.height, 16);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneColorTexture));
                    shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(autoExposureHistogramTexture, 0));

                    commandList.ClearTextureUAV(RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(autoExposureHistogramTexture), 0), RenderBackendTextureClearValue::CreateColorValueFloat4(0.0f, 0.0f, 0.0f, 0.0f));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::AutoExposureBuildHistogram);

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

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(autoExposureHistogramTexture));
                    shaderConstants.BindBufferSRV(2, registry.GetBufferSRVBindlessResourceDescriptorIndex(previousAutoExposureBuffer));
                    shaderConstants.BindBufferUAV(3, registry.GetBufferUAVBindlessResourceDescriptorIndex(autoExposureBuffer));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::AutoExposureComputeExposure);

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

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    commandList.CopyBuffer(registry.GetRenderBackendBufferHandle(autoExposureBuffer), 0, registry.GetRenderBackendBufferHandle(autoExposureReadbackBuffer), 0, sizeof(AutoExposureData));
                };
            });

        renderGraph.ExportBufferDeferred(autoExposureBuffer, &historyFrame.autoExposureBuffer);

        return autoExposureBuffer;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddCopyExposurePass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphBufferHandle autoExposureBuffer)
    {
        RenderGraphTextureDesc exposureTextureDesc = RenderGraphTextureDesc::Create2D(
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

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, registry.GetBufferSRVBindlessResourceDescriptorIndex(autoExposureBuffer));
                    shaderConstants.BindTextureUAV(1, registry.GetTextureUAVBindlessResourceDescriptorIndex(exposureTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::CopyExposure);

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