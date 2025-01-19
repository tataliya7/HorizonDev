#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    bool RealTimeRenderer::IsLocalExposureEnabled() const
    {
        return features.enableLocalExposure;
    }

    RenderGraphTextureHandle RealTimeRenderer::DispatchLocalExposure(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle colorTexture,
        RenderGraphTextureHandle autoExposureTexture)
    {
        const PostProcessingSettings& postProcessingSettings = view.renderSettings.postProcessingSettings;

        float highlights = 1.0f;//std::pow(2.0f, -postProcessingSettings.localExposureHighlights);
        float shadows = 0.0f;//std::pow(2.0f, postProcessingSettings.localExposureShadows);
        float sigma = 0.25f;//postProcessingSettings.localExposurePreferenceSigma * postProcessingSettings.localExposurePreferenceSigma;
        int32 coarsestMipLevel = 0;//postProcessingSettings.localExposureCoarsestMipLevel;
        int32 displayMipLevel = 1;//postProcessingSettings.localExposureDisplayMipLevel;

        bool isAutoExposureTextureValid = !autoExposureTexture.IsNullHandle();

        uint32 downsampleFactor = 2;

        uint32 width = std::max(1u, Math::CeilDiv(targetResolution.width, downsampleFactor));
        uint32 height = std::max(1u, Math::CeilDiv(targetResolution.height, downsampleFactor));
        uint32 mipLevelCount = Math::MaxMipLevelCount(width, height);

        coarsestMipLevel = std::clamp(coarsestMipLevel, 0, (int)mipLevelCount - 1);
        displayMipLevel = std::clamp(std::min(displayMipLevel, coarsestMipLevel), 0, (int)mipLevelCount - 1);

        RenderGraphTextureDesc localExposureTextureDesc = RenderGraphTextureDesc::Create2D(
            width,
            height,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            RenderBackendTextureClearValue::Black,
            mipLevelCount);

        RenderGraphTextureHandle localExposureLuminance = renderGraph.CreateTexture(localExposureTextureDesc, "LocalExposureLuminance");
        RenderGraphTextureHandle localExposureWeights = renderGraph.CreateTexture(localExposureTextureDesc, "LocalExposureWeights");
        RenderGraphTextureHandle localExposureAssemble = renderGraph.CreateTexture(localExposureTextureDesc, "LocalExposureAssemble");
        RenderGraphTextureHandle localExposureTexture = renderGraph.CreateTexture(localExposureTextureDesc, "LocalExposureTexture");

        renderGraph.AddPass(
            std::format("LocalExposureComputeLuminance (Compute, {}x{})", width, height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                colorTexture = builder.ReadTexture(colorTexture, RenderBackendResourceState::ShaderResource);
                if (isAutoExposureTextureValid) builder.ReadTexture(autoExposureTexture, RenderBackendResourceState::ShaderResource);

                localExposureLuminance = builder.WriteTexture(localExposureLuminance, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(colorTexture));
                    if (isAutoExposureTextureValid) shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(autoExposureTexture));
                    shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndex(localExposureLuminance, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LocalExposureComputeLuminance);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("LocalExposureComputeWeights (Compute, {}x{})", width, height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(localExposureLuminance, RenderBackendResourceState::ShaderResource);

                localExposureWeights = builder.WriteTexture(localExposureWeights, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(localExposureLuminance));
                    shaderConstants.BindTextureUAV(2, registry.GetTextureUAVBindlessResourceDescriptorIndex(localExposureWeights, 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LocalExposureComputeWeights);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });
#if 0
        renderGraph.AddPass(
            std::format("LocalExposureGenerateMipChain (Compute, {}x{})", width, height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                localExposureLuminance = builder.ReadWriteTexture(localExposureLuminance, RenderBackendResourceState::ShaderResource);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 w = width;
                    uint32 h = height;
                    RenderBackendTextureHandle textureHandle = registry.GetRenderBackendTextureHandle(localExposureLuminance);

                    RenderBackendShaderHandle downsampleTexture2DCS = shaderLibrary->GetShader(ShaderID::DownsampleTexture);

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

                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(textureHandle));
                        shaderConstants.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(textureHandle, mipLevel));
                        shaderConstants.PushConstants(0, (float)(mipLevel - 1));

                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
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
            std::format("LocalExposureGenerateMipChain (Compute, {}x{})", width, height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                localExposureWeights = builder.ReadWriteTexture(localExposureWeights, RenderBackendResourceState::ShaderResource);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 w = width;
                    uint32 h = height;
                    RenderBackendTextureHandle textureHandle = registry.GetRenderBackendTextureHandle(localExposureWeights);

                    RenderBackendShaderHandle downsampleTexture2DCS = renderEngine->GetShaderLibrary()->GetShader((uint32)ShaderPipelineID::DownsampleTexture2D);

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

                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(textureHandle));
                        shaderConstants.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(textureHandle, mipLevel));
                        //shaderConstants.PushConstants(0, (float)(mipLevel - 1));

                        commandList.Dispatch(
                            downsampleTexture2DCS,
                            shaderConstants,
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
            std::format("LocalExposureBlendExposures (Compute, {}x{})", coarsestMipLevelWidth, coarsestMipLevelHeight),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(localExposureLuminance, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(localExposureWeights, RenderBackendResourceState::ShaderResource);

                localExposureAssemble = builder.WriteTexture(localExposureAssemble, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(coarsestMipLevelWidth, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(coarsestMipLevelHeight, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(localExposureLuminance));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(localExposureWeights));
                    shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndex(localExposureAssemble, coarsestMipLevel));
                    //shaderConstants.PushConstants(0, (float)coarsestMipLevel);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LocalExposureBlendExposures);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("LocalExposureBlendLaplacian (Compute, {}x{})", width, height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(localExposureLuminance, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(localExposureWeights, RenderBackendResourceState::ShaderResource);

                localExposureAssemble = builder.ReadWriteTexture(localExposureAssemble, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 w = coarsestMipLevelWidth;
                    uint32 h = coarsestMipLevelHeight;

                    for (int32 mipLevel = coarsestMipLevel; mipLevel > displayMipLevel; mipLevel--)
                    {
                        RenderBackendBarrier barriers[] =
                        {
                            RenderBackendBarrier(registry.GetRenderBackendTextureHandle(localExposureAssemble), RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
                        };
                        commandList.Barriers(barriers, 1);

                        w = w << 1;
                        h = h << 1;

                        uint32 threadGroupCountX = ComputeShaderThreadGroupCount(w, PostProcessingThreadGroupSizeX);
                        uint32 threadGroupCountY = ComputeShaderThreadGroupCount(h, PostProcessingThreadGroupSizeY);
                        uint32 threadGroupCountZ = 1;

                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                        shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(localExposureLuminance));
                        shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(localExposureWeights));
                        shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(localExposureAssemble));
                        shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndex(localExposureAssemble, mipLevel - 1));
                        //shaderConstants.PushConstants(0, (float)mipLevel);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LocalExposureBlendLaplacian);

                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    }

                    if ((coarsestMipLevel - displayMipLevel > 0) && (coarsestMipLevel < (int)mipLevelCount - 1))
                    {
                        RenderBackendBarrier barriers[] =
                        {
                            RenderBackendBarrier(registry.GetRenderBackendTextureHandle(localExposureAssemble), RenderBackendTextureSubresourceRange(displayMipLevel + 1, coarsestMipLevel - displayMipLevel, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::ShaderResource, RenderBackendResourceState::UnorderedAccess),
                        };
                        commandList.Barriers(barriers, 1);
                    }
                };
            });

        uint32 displayMipLevelWidth = width >> displayMipLevel;
        uint32 displayMipLevelHeight = height >> displayMipLevel;

        renderGraph.AddPass(
            std::format("LocalExposureGuidedUpsampling (Compute, {}x{} -> {}x{})", displayMipLevelWidth, displayMipLevelHeight, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(colorTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(localExposureLuminance, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(localExposureAssemble, RenderBackendResourceState::ShaderResource);

                localExposureAssemble = builder.WriteTexture(localExposureAssemble, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(colorTexture));
                    if (isAutoExposureTextureValid) shaderConstants.BindTextureSRV(5, registry.GetTextureSRVBindlessResourceDescriptorIndex(autoExposureTexture));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(localExposureLuminance));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(localExposureAssemble));
                    shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndex(localExposureTexture, 0));
                    ///shaderConstants.PushConstants(0, (float)displayMipLevel);
                    ///shaderConstants.PushConstants(1, (float)displayMipLevelWidth);
                    ///shaderConstants.PushConstants(2, (float)displayMipLevelHeight);
                    ///shaderConstants.PushConstants(3, 1.0f / (float)displayMipLevelWidth);
                    ///shaderConstants.PushConstants(4, 1.0f / (float)displayMipLevelHeight);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LocalExposureGuidedUpsampling);

                    commandList.Dispatch(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return localExposureTexture;
    }
}