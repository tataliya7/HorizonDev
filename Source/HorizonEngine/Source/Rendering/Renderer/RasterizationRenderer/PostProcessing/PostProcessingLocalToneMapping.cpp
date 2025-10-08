#include "../RasterizationRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    bool RasterizationRenderer::IsLocalToneMappingEnabled() const
    {
        return renderFeatures.enableBilateralGridLocalToneMapping;
    }

    RenderGraphTextureHandle RasterizationRenderer::DispatchBilateralGridLocalToneMapping(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle colorTexture,
        const PostProcessingColorPyramid& colorPyramid,
        RenderGraphBufferHandle autoExposureBuffer)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "LocalToneMapping");

        const RasterizationRendererPostProcessingSettings& postProcessingSettings = rendererSettings.postProcessingSettings;

        const uint32 bilateralGridTextureWidth = Math::CeilDiv(renderResolution.width, 8 * 8);
        const uint32 bilateralGridTextureHeight = Math::CeilDiv(renderResolution.height, 8 * 8);
        const uint32 bilateralGridTextureDepth = 64;

        RenderGraphTextureDescription gridTextureDesc = RenderGraphTextureDescription::Create3D(
            bilateralGridTextureWidth,
            bilateralGridTextureHeight,
            bilateralGridTextureDepth,
            RenderBackendTextureFormat::R32G32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);

        RenderGraphTextureHandle gridTexture = renderGraph.CreateTexture(gridTextureDesc, "BilateralGridLocalToneMappingGridTexture");
        RenderGraphTextureHandle bilaterallyFilteredGridTexture = renderGraph.CreateTexture(gridTextureDesc, "BilateralGridLocalToneMappingBilaterallyFilteredGridTexture");

        renderGraph.AddPass(
            std::format("BilateralGridLocalToneMappingBuildGrid (Compute, {}x{}x{})", bilateralGridTextureWidth, bilateralGridTextureHeight, bilateralGridTextureDepth),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                colorTexture = builder.ReadTexture(colorTexture, RenderBackendResourceState::ShaderResource);
                gridTexture = builder.WriteTexture(gridTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = bilateralGridTextureWidth;
                    uint32 threadGroupCountY = bilateralGridTextureHeight;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(colorTexture));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(gridTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::BilateralGridLocalToneMappingBuildGrid);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        bilaterallyFilteredGridTexture = gridTexture;

        RenderGraphTextureHandle gaussianFilterInputColorTexture = colorPyramid.textures[4];
        const RenderGraphTextureDescription gaussianFilterInputColorTextureDesc = renderGraph.GetTextureDescription(gaussianFilterInputColorTexture);

        RenderGraphTextureDescription gaussianFilteredLogLuminanceTextureDesc = RenderGraphTextureDescription::Create2D(
            gaussianFilterInputColorTextureDesc.width,
            gaussianFilterInputColorTextureDesc.height,
            RenderBackendTextureFormat::R16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);

        RenderGraphTextureHandle lowResolutionLogLuminanceTexture = renderGraph.CreateTexture(gaussianFilteredLogLuminanceTextureDesc, "LowResolutionLogLuminanceTexture");
        RenderGraphTextureHandle intermediateGaussianFilteredLogLuminanceTexture = renderGraph.CreateTexture(gaussianFilteredLogLuminanceTextureDesc, "IntermediateGaussianFilteredLogLuminanceTexture");
        RenderGraphTextureHandle gaussianFilteredLogLuminanceTexture = renderGraph.CreateTexture(gaussianFilteredLogLuminanceTextureDesc, "GaussianFilteredLogLuminanceTexture");

        renderGraph.AddPass(
            std::format("BilateralGridLocalToneMappingComputeLogLuminance (Compute, {}x{}x{})", gaussianFilteredLogLuminanceTextureDesc.width, gaussianFilteredLogLuminanceTextureDesc.height, 1),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                gaussianFilterInputColorTexture = builder.ReadTexture(gaussianFilterInputColorTexture, RenderBackendResourceState::ShaderResource);
                lowResolutionLogLuminanceTexture = builder.WriteTexture(lowResolutionLogLuminanceTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(gaussianFilteredLogLuminanceTextureDesc.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(gaussianFilteredLogLuminanceTextureDesc.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gaussianFilterInputColorTexture));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(lowResolutionLogLuminanceTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::BilateralGridLocalToneMappingComputeLogLuminance);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        // 2D Gaussian blur must be wide enough to avoid noticeable haloing.
        const uint32 GaussianFilterMaxRadius = 32u;
        const uint32 GaussianFilterMaxKernelSize = 2 * GaussianFilterMaxRadius + 1;

        float radius = float(std::max(gaussianFilterInputColorTextureDesc.width, gaussianFilterInputColorTextureDesc.height)) * 0.25f;
        const uint32 gaussianFilterRadius = std::clamp(uint32(std::ceil(radius)), 1u, GaussianFilterMaxRadius);

        const uint32 gaussianFilterKernelSize = 2 * gaussianFilterRadius + 1;
        const float gaussianFilterSigma = 0.3f * (float(gaussianFilterKernelSize - 1) * 0.5f - 1.0f) + 0.8f;

        RenderGraphBufferDescription gaussianDistributionBufferDesc = RenderGraphBufferDescription::CreateByteAddress(2 * sizeof(float) * GaussianFilterMaxKernelSize);
        RenderGraphBufferHandle gaussianDistributionBuffer = renderGraph.CreateBuffer(gaussianDistributionBufferDesc, "GaussianDistributionBuffer");

        renderGraph.AddPass(
            std::format("BilateralGridLocalToneMappingGaussianDistribution (Compute, {}x{}x{})", gaussianFilterKernelSize, 1, 1),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                gaussianDistributionBuffer = builder.WriteBuffer(gaussianDistributionBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = 1;
                    uint32 threadGroupCountY = 1;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferUAV(0, resourceRegistry.GetBufferUAVBindlessResourceDescriptorIndex(gaussianDistributionBuffer));
                    pushConstantValues.OverrideShaderConstantValue(1, gaussianFilterKernelSize);
                    pushConstantValues.OverrideShaderConstantValue(2, gaussianFilterSigma);

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::BilateralGridLocalToneMappingGaussianDistribution);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("BilateralGridLocalToneMappingGaussianFilter-Horizontal (Compute, {}x{}x{})", gaussianFilteredLogLuminanceTextureDesc.width, gaussianFilteredLogLuminanceTextureDesc.height, 1),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                gaussianDistributionBuffer = builder.ReadBuffer(gaussianDistributionBuffer, RenderBackendResourceState::ShaderResource);
                lowResolutionLogLuminanceTexture = builder.ReadTexture(lowResolutionLogLuminanceTexture, RenderBackendResourceState::ShaderResource);
                intermediateGaussianFilteredLogLuminanceTexture = builder.WriteTexture(intermediateGaussianFilteredLogLuminanceTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(gaussianFilteredLogLuminanceTextureDesc.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(gaussianFilteredLogLuminanceTextureDesc.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(gaussianDistributionBuffer));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(lowResolutionLogLuminanceTexture));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(intermediateGaussianFilteredLogLuminanceTexture, 0));
                    pushConstantValues.OverrideShaderConstantValue(3, 0u);
                    pushConstantValues.OverrideShaderConstantValue(4, gaussianFilterKernelSize);

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::BilateralGridLocalToneMappingGaussianFilter);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("BilateralGridLocalToneMappingGaussianFilter-Vertical (Compute, {}x{}x{})", gaussianFilteredLogLuminanceTextureDesc.width, gaussianFilteredLogLuminanceTextureDesc.height, 1),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                intermediateGaussianFilteredLogLuminanceTexture = builder.ReadTexture(intermediateGaussianFilteredLogLuminanceTexture, RenderBackendResourceState::ShaderResource);
                gaussianFilteredLogLuminanceTexture = builder.WriteTexture(gaussianFilteredLogLuminanceTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(gaussianFilteredLogLuminanceTextureDesc.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(gaussianFilteredLogLuminanceTextureDesc.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(gaussianDistributionBuffer));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(intermediateGaussianFilteredLogLuminanceTexture));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(gaussianFilteredLogLuminanceTexture, 0));
                    pushConstantValues.OverrideShaderConstantValue(3, 1u);
                    pushConstantValues.OverrideShaderConstantValue(4, gaussianFilterKernelSize);

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::BilateralGridLocalToneMappingGaussianFilter);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        RenderGraphTextureDescription localToneMappingTextureDesc = RenderGraphTextureDescription::Create2D(
            targetResolution.width,
            targetResolution.height,
            RenderBackendTextureFormat::R32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);

        RenderGraphTextureHandle localToneMappingTexture = renderGraph.CreateTexture(localToneMappingTextureDesc, "LocalToneMappingTexture");

        renderGraph.AddPass(
            std::format("BilateralGridLocalToneMappingUpsampling (Compute, {}x{}x{})", localToneMappingTextureDesc.width, localToneMappingTextureDesc.height, 1),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                colorTexture = builder.ReadTexture(colorTexture, RenderBackendResourceState::ShaderResource);
                bilaterallyFilteredGridTexture = builder.ReadTexture(bilaterallyFilteredGridTexture, RenderBackendResourceState::ShaderResource);
                gaussianFilteredLogLuminanceTexture = builder.ReadTexture(gaussianFilteredLogLuminanceTexture, RenderBackendResourceState::ShaderResource);
                autoExposureBuffer = builder.ReadBuffer(autoExposureBuffer, RenderBackendResourceState::ShaderResource);
                localToneMappingTexture = builder.WriteTexture(localToneMappingTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(localToneMappingTextureDesc.width, 8);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(localToneMappingTextureDesc.height, 8);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(colorTexture));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(bilaterallyFilteredGridTexture));
                    pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gaussianFilteredLogLuminanceTexture));
                    pushConstantValues.BindBufferSRV(4, resourceRegistry.GetBufferSRVBindlessResourceDescriptorIndex(autoExposureBuffer));
                    pushConstantValues.BindTextureUAV(5, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(localToneMappingTexture, 0));
                    pushConstantValues.OverrideShaderConstantValue(6, postProcessingSettings.bilateralGridLocalToneMappingShadows);
                    pushConstantValues.OverrideShaderConstantValue(7, postProcessingSettings.bilateralGridLocalToneMappingHighlights);
                    pushConstantValues.OverrideShaderConstantValue(8, postProcessingSettings.bilateralGridLocalToneMappingDetailStrength);
                    pushConstantValues.OverrideShaderConstantValue(9, postProcessingSettings.bilateralGridLocalToneMappingGaussianFilterWeight);
                    pushConstantValues.OverrideShaderConstantValue(10, 1.0f / float(bilateralGridTextureWidth));
                    pushConstantValues.OverrideShaderConstantValue(11, 1.0f / float(bilateralGridTextureHeight));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::BilateralGridLocalToneMappingUpsampling);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return localToneMappingTexture;
    }

    RenderGraphTextureHandle RasterizationRenderer::DispatchExposureFusionLocalToneMapping(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle colorTexture,
        const PostProcessingColorPyramid& colorPyramid,
        RenderGraphTextureHandle exposureTexture)
    {
        const RasterizationRendererPostProcessingSettings& postProcessingSettings = rendererSettings.postProcessingSettings;

        float highlights = 1.0f;//std::pow(2.0f, -postProcessingSettings.localToneMappingHighlights);
        float shadows = 0.0f;//std::pow(2.0f, postProcessingSettings.localToneMappingShadows);
        float sigma = 0.25f;//postProcessingSettings.localToneMappingPreferenceSigma * postProcessingSettings.localToneMappingPreferenceSigma;
        int32 coarsestMipLevel = 0;//postProcessingSettings.localToneMappingCoarsestMipLevel;
        int32 displayMipLevel = 1;//postProcessingSettings.localToneMappingDisplayMipLevel;

        uint32 downsampleFactor = 2;

        uint32 width = std::max(1u, Math::CeilDiv(targetResolution.width, downsampleFactor));
        uint32 height = std::max(1u, Math::CeilDiv(targetResolution.height, downsampleFactor));
        uint32 mipLevelCount = Math::MaxMipLevelCount(width, height);

        coarsestMipLevel = std::clamp(coarsestMipLevel, 0, (int)mipLevelCount - 1);
        displayMipLevel = std::clamp(std::min(displayMipLevel, coarsestMipLevel), 0, (int)mipLevelCount - 1);

        RenderGraphTextureDescription localToneMappingTextureDesc = RenderGraphTextureDescription::Create2D(
            width,
            height,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            RenderBackendTextureClearValue::Black,
            mipLevelCount);

        RenderGraphTextureDescription exposureFusionLuminanceTextureDesc = RenderGraphTextureDescription::Create2D(
           width,
           height,
           RenderBackendTextureFormat::R11G11B10Float,
           RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
           RenderBackendTextureClearValue::Black,
           mipLevelCount);
        RenderGraphTextureDescription exposureFusionWeightTextureDesc = exposureFusionLuminanceTextureDesc;

        RenderGraphTextureHandle exposureFusionLuminanceTexture = renderGraph.CreateTexture(exposureFusionLuminanceTextureDesc, "ExposureFusionLuminanceTexture");
        RenderGraphTextureHandle exposureFusionWeightTexture = renderGraph.CreateTexture(exposureFusionWeightTextureDesc, "ExposureFusionWeightTexture");
        RenderGraphTextureHandle localToneMappingAssemble = renderGraph.CreateTexture(localToneMappingTextureDesc, "LocalToneMappingAssemble");
        RenderGraphTextureHandle localToneMappingTexture = renderGraph.CreateTexture(localToneMappingTextureDesc, "LocalToneMappingTexture");

        renderGraph.AddPass(
            std::format("ExposureFusionComputeLuminanceAndWeight (Compute, {}x{})", width, height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                colorTexture = builder.ReadTexture(colorTexture, RenderBackendResourceState::ShaderResource);
                exposureTexture = builder.ReadTexture(exposureTexture, RenderBackendResourceState::ShaderResource);
                exposureFusionLuminanceTexture = builder.WriteTexture(exposureFusionLuminanceTexture, RenderBackendResourceState::UnorderedAccess);
                exposureFusionWeightTexture = builder.WriteTexture(exposureFusionWeightTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(colorTexture));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(exposureTexture));
                    pushConstantValues.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(exposureFusionLuminanceTexture, 0));
                    pushConstantValues.BindTextureUAV(4, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(exposureFusionWeightTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::ExposureFusionComputeLuminanceAndWeight);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

#if 0
        renderGraph.AddPass(
            std::format("LocalToneMappingGenerateMipChain (Compute, {}x{})", width, height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                localToneMappingLuminance = builder.ReadWriteTexture(localToneMappingLuminance, RenderBackendResourceState::ShaderResource);


                {
                    uint32 w = width;
                    uint32 h = height;
                    RenderBackendTextureHandle textureHandle = resourceRegistry.GetRenderBackendTextureHandle(localToneMappingLuminance);

                    RenderBackendShaderHandle downsampleTexture2DCS = shaderRepository->GetShader(ShaderID::DownsampleTexture);

                    for (uint32 mipLevel = 1; mipLevel < mipLevelCount; mipLevel++)
                    {
                        if (mipLevel == 1)
                        {
                            RenderBackendBarrier barriers[] =
                            {
                                RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
                            };
                            commandList.Barriers(barriers, 1);
                        }
                        else
                        {
                            RenderBackendBarrier barriers[] =
                            {
                                RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
                                RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
                            };
                            commandList.Barriers(barriers, 2);
                        }

                        w = w >> 1;
                        h = h >> 1;

                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(w, 8);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(h, 8);

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(textureHandle));
                        pushConstantValues.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(textureHandle, mipLevel));
                        pushConstantValues.PushConstants(0, (float)(mipLevel - 1));

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    }
                    RenderBackendBarrier transition = RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevelCount - 1, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
                    commandList.Barriers(&transition, 1);
                };
            });


        // TODO: Better downsampling filter? @see https://bartwronski.com/2021/07/20/processing-aware-image-filtering-compensating-for-the-upsampling/

        // TODO: Create mips in one pass
        renderGraph.AddPass(
            std::format("LocalToneMappingGenerateMipChain (Compute, {}x{})", width, height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                localToneMappingWeights = builder.ReadWriteTexture(localToneMappingWeights, RenderBackendResourceState::ShaderResource);


                {
                    uint32 w = width;
                    uint32 h = height;
                    RenderBackendTextureHandle textureHandle = resourceRegistry.GetRenderBackendTextureHandle(localToneMappingWeights);

                    RenderBackendShaderHandle downsampleTexture2DCS = renderEngine->GetShaderRepository()->GetShader((uint32)ShaderPipelineID::DownsampleTexture2D);

                    for (uint32 mipLevel = 1; mipLevel < mipLevelCount; mipLevel++)
                    {
                        if (mipLevel == 1)
                        {
                            RenderBackendBarrier barriers[] =
                            {
                                RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
                            };
                            commandList.Barriers(barriers, 1);
                        }
                        else
                        {
                            RenderBackendBarrier barriers[] =
                            {
                                RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
                                RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
                            };
                            commandList.Barriers(barriers, 2);
                        }

                        w = w >> 1;
                        h = h >> 1;

                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(w, 8);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(h, 8);
                        uint32 threadGroupCountZ = 1;

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(textureHandle));
                        pushConstantValues.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(textureHandle, mipLevel));
                        //pushConstantValues.PushConstants(0, (float)(mipLevel - 1));

                        commandList.Dispatch(
                            downsampleTexture2DCS,
                            pushConstantValues,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    }
                    RenderBackendBarrier transition = RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevelCount - 1, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
                    commandList.Barriers(&transition, 1);
                };
            });
#endif
        uint32 coarsestMipLevelWidth = width >> coarsestMipLevel;
        uint32 coarsestMipLevelHeight = height >> coarsestMipLevel;

        // Blend the coarsest level - Gaussian.
        renderGraph.AddPass(
            std::format("LocalToneMappingBlendExposures (Compute, {}x{})", coarsestMipLevelWidth, coarsestMipLevelHeight),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(exposureFusionLuminanceTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(exposureFusionWeightTexture, RenderBackendResourceState::ShaderResource);

                localToneMappingAssemble = builder.WriteTexture(localToneMappingAssemble, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(coarsestMipLevelWidth, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(coarsestMipLevelHeight, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(exposureFusionLuminanceTexture));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(exposureFusionWeightTexture));
                    pushConstantValues.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(localToneMappingAssemble, coarsestMipLevel));
                    //pushConstantValues.PushConstants(0, (float)coarsestMipLevel);

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::LocalToneMappingBlendExposures);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("LocalToneMappingBlendLaplacian (Compute, {}x{})", width, height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(exposureFusionLuminanceTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(exposureFusionWeightTexture, RenderBackendResourceState::ShaderResource);

                localToneMappingAssemble = builder.ReadWriteTexture(localToneMappingAssemble, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 w = coarsestMipLevelWidth;
                    uint32 h = coarsestMipLevelHeight;

                    for (int32 mipLevel = coarsestMipLevel; mipLevel > displayMipLevel; mipLevel--)
                    {
                        RenderBackendBarrier barriers[] =
                        {
                            RenderBackendBarrier(resourceRegistry.GetRenderBackendTextureHandle(localToneMappingAssemble), RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
                        };
                        commandList.Barriers(barriers, 1);

                        w = w << 1;
                        h = h << 1;

                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(w, PostProcessingThreadGroupSizeX);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(h, PostProcessingThreadGroupSizeY);
                        uint32 threadGroupCountZ = 1;

                        RenderBackendPushConstantValues pushConstantValues = {};
                        pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(exposureFusionLuminanceTexture));
                        pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(exposureFusionWeightTexture));
                        pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(localToneMappingAssemble));
                        pushConstantValues.BindTextureUAV(4, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(localToneMappingAssemble, mipLevel - 1));
                        //pushConstantValues.PushConstants(0, (float)mipLevel);

                        RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::LocalToneMappingBlendLaplacian);

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    }

                    if ((coarsestMipLevel - displayMipLevel > 0) && (coarsestMipLevel < (int)mipLevelCount - 1))
                    {
                        RenderBackendBarrier barriers[] =
                        {
                            RenderBackendBarrier(resourceRegistry.GetRenderBackendTextureHandle(localToneMappingAssemble), RenderBackendTextureSubresourceRange(displayMipLevel + 1, coarsestMipLevel - displayMipLevel, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::ShaderResource, RenderBackendResourceState::UnorderedAccess),
                        };
                        commandList.Barriers(barriers, 1);
                    }
                };
            });

        uint32 displayMipLevelWidth = width >> displayMipLevel;
        uint32 displayMipLevelHeight = height >> displayMipLevel;

        renderGraph.AddPass(
            std::format("ExposureFusionGuidedUpsampling (Compute, {}x{} -> {}x{})", displayMipLevelWidth, displayMipLevelHeight, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(colorTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(exposureFusionLuminanceTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(localToneMappingAssemble, RenderBackendResourceState::ShaderResource);

                localToneMappingAssemble = builder.WriteTexture(localToneMappingAssemble, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(colorTexture));
                    pushConstantValues.BindTextureSRV(5, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(exposureTexture));
                    pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(exposureFusionLuminanceTexture));
                    pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(localToneMappingAssemble));
                    pushConstantValues.BindTextureUAV(4, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(localToneMappingTexture, 0));
                    ///pushConstantValues.PushConstants(0, (float)displayMipLevel);
                    ///pushConstantValues.PushConstants(1, (float)displayMipLevelWidth);
                    ///pushConstantValues.PushConstants(2, (float)displayMipLevelHeight);
                    ///pushConstantValues.PushConstants(3, 1.0f / (float)displayMipLevelWidth);
                    ///pushConstantValues.PushConstants(4, 1.0f / (float)displayMipLevelHeight);

                    RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::ExposureFusionGuidedUpsampling);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return localToneMappingTexture;
    }
}