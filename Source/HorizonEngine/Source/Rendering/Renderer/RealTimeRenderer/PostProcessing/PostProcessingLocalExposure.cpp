#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
#if 0
    RenderGraphTextureHandle RealTimeRenderer::AddLocalExposurePass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture,
        RenderGraphTextureHandle autoExposureTexture)
    {
        float highlights = std::pow(2.0f, -settings.postProcessingSettings.localExposureHighlights);
        float shadows = std::pow(2.0f, settings.postProcessingSettings.localExposureShadows);
        float sigma = settings.postProcessingSettings.localExposurePreferenceSigma * settings.postProcessingSettings.localExposurePreferenceSigma;
        int32 coarsestMipLevel = settings.postProcessingSettings.localExposureCoarsestMipLevel;
        int32 displayMipLevel = settings.postProcessingSettings.localExposureDisplayMipLevel;

        bool isAutoExposureTextureValid = !autoExposureTexture.IsNullHandle();

        uint32 downsampleFactor = 2;

        uint32 width = std::max(1u, ComputeWorkGroupCount(targetResolution.width, downsampleFactor));
        uint32 height = std::max(1u, ComputeWorkGroupCount(targetResolution.height, downsampleFactor));
        uint32 mipLevels = Math::MaxNumMipLevels(width, height);

        coarsestMipLevel = std::clamp(coarsestMipLevel, 0, (int)mipLevels - 1);
        displayMipLevel = std::clamp(std::min(displayMipLevel, coarsestMipLevel), 0, (int)mipLevels - 1);

        RenderGraphTextureDesc localExposureLuminancesDesc = RenderGraphTextureDesc::Create2D(
            width,
            height,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess,
            RenderBackendTextureClearValue::Black,
            mipLevels);
        RenderGraphTextureHandle localExposureLuminances = renderGraph.CreateTexture(localExposureLuminancesDesc, "LocalExposureLuminances");

        RenderGraphTextureHandle localExposureWeights = renderGraph.CreateTexture(localExposureLuminancesDesc, "LocalExposureWeights");

        RenderGraphTextureHandle localExposureAssemble = renderGraph.CreateTexture(localExposureLuminancesDesc, "LocalExposureAssemble");

        RenderGraphTextureHandle localExposureTexture = renderGraph.CreateTexture(localExposureLuminancesDesc, "LocalExposureTexture");

        renderGraph.AddPass(std::format("LocalExposureComputeLuminances (Compute, {}x{})", width, height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);
                if (isAutoExposureTextureValid) builder.ReadTexture(autoExposureTexture, RenderBackendResourceState::ShaderResource);

                localExposureLuminances = builder.WriteTexture(localExposureLuminances, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 threadGroupCountX = ComputeWorkGroupCount(width, PostProcessingThreadGroupSizeX);
                        uint32 threadGroupCountY = ComputeWorkGroupCount(height, PostProcessingThreadGroupSizeY);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorTexture)));
                        if (isAutoExposureTextureValid) shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(autoExposureTexture)));
                        shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureLuminances), 0));
                        shaderArguments.PushConstants(0, highlights);
                        shaderArguments.PushConstants(1, shadows);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LocalExposureComputeLuminances);
                        commandList.Dispatch2D(
                            computeShader,
                            shaderArguments,
                            threadGroupCountX,
                            groupCountY);
                    };
            });

        renderGraph.AddPass(std::format("LocalExposureComputeWeights (Compute, {}x{})", width, height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(localExposureLuminances, RenderBackendResourceState::ShaderResource);

                localExposureWeights = builder.WriteTexture(localExposureWeights, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 threadGroupCountX = ComputeWorkGroupCount(width, PostProcessingThreadGroupSizeX);
                        uint32 threadGroupCountY = ComputeWorkGroupCount(height, PostProcessingThreadGroupSizeY);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureLuminances)));
                        shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureWeights), 0));
                        shaderArguments.PushConstants(0, sigma);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LocalExposureComputeWeights);
                        commandList.Dispatch2D(
                            computeShader,
                            shaderArguments,
                            threadGroupCountX,
                            groupCountY);
                    };
            });

        renderGraph.AddPass(std::format("LocalExposureGenerateMipChain (Compute, {}x{})", width, height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                localExposureLuminances = builder.ReadWriteTexture(localExposureLuminances, RenderBackendResourceState::ShaderResource);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 w = width;
                        uint32 h = height;
                        RenderBackendTextureHandle textureHandle = registry.GetRenderBackendTextureHandle(localExposureLuminances);
                        RenderBackendShaderHandle downsampleTexture2DCS = renderEngine->GetShaderLibrary()->GetShader((uint32)ShaderPipelineID::DownsampleTexture2D);
                        for (uint32 mipLevel = 1; mipLevel < mipLevels; mipLevel++)
                        {
                            if (mipLevel == 1)
                            {
                                RenderBackendBarrier transitions[] =
                                {
                                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
                                };
                                commandList.Transitions(transitions, 1);
                            }
                            else
                            {
                                RenderBackendBarrier transitions[] =
                                {
                                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
                                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
                                };
                                commandList.Transitions(transitions, 2);
                            }

                            w = w >> 1;
                            h = h >> 1;

                            uint32 threadGroupCountX = ComputeWorkGroupCount(w, 8);
                            uint32 threadGroupCountY = ComputeWorkGroupCount(h, 8);

                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(textureHandle));
                            shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(textureHandle, mipLevel));
                            shaderArguments.PushConstants(0, (float)(mipLevel - 1));

                            commandList.Dispatch2D(
                                downsampleTexture2DCS,
                                shaderArguments,
                                threadGroupCountX,
                                groupCountY);
                        }
                        RenderBackendBarrier transition = RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevels - 1, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
                        commandList.Transitions(&transition, 1);
                    };
            });

        // TODO: Better downsampling filter? @see https://bartwronski.com/2021/07/20/processing-aware-image-filtering-compensating-for-the-upsampling/

        // TODO: Create mips in one pass
        renderGraph.AddPass(std::format("LocalExposureGenerateMipChain (Compute, {}x{})", width, height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                localExposureWeights = builder.ReadWriteTexture(localExposureWeights, RenderBackendResourceState::ShaderResource);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 w = width;
                        uint32 h = height;
                        RenderBackendTextureHandle textureHandle = registry.GetRenderBackendTextureHandle(localExposureWeights);
                        RenderBackendShaderHandle downsampleTexture2DCS = renderEngine->GetShaderLibrary()->GetShader((uint32)ShaderPipelineID::DownsampleTexture2D);
                        for (uint32 mipLevel = 1; mipLevel < mipLevels; mipLevel++)
                        {
                            if (mipLevel == 1)
                            {
                                RenderBackendBarrier transitions[] =
                                {
                                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
                                };
                                commandList.Transitions(transitions, 1);
                            }
                            else
                            {
                                RenderBackendBarrier transitions[] =
                                {
                                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
                                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
                                };
                                commandList.Transitions(transitions, 2);
                            }

                            w = w >> 1;
                            h = h >> 1;

                            uint32 threadGroupCountX = ComputeWorkGroupCount(w, 8);
                            uint32 threadGroupCountY = ComputeWorkGroupCount(h, 8);

                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(textureHandle));
                            shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(textureHandle, mipLevel));
                            shaderArguments.PushConstants(0, (float)(mipLevel - 1));

                            commandList.Dispatch2D(
                                downsampleTexture2DCS,
                                shaderArguments,
                                threadGroupCountX,
                                groupCountY);
                        }
                        RenderBackendBarrier transition = RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevels - 1, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
                        commandList.Transitions(&transition, 1);
                    };
            });

        uint32 coarsestMipLevelWidth = width >> coarsestMipLevel;
        uint32 coarsestMipLevelHeight = height >> coarsestMipLevel;

        // Blend the coarsest level - Gaussian.
        renderGraph.AddPass(std::format("LocalExposureBlendExposures (Compute, {}x{})", coarsestMipLevelWidth, coarsestMipLevelHeight), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(localExposureLuminances, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(localExposureWeights, RenderBackendResourceState::ShaderResource);

                localExposureAssemble = builder.WriteTexture(localExposureAssemble, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 threadGroupCountX = ComputeWorkGroupCount(coarsestMipLevelWidth, PostProcessingThreadGroupSizeX);
                        uint32 threadGroupCountY = ComputeWorkGroupCount(coarsestMipLevelHeight, PostProcessingThreadGroupSizeY);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureLuminances)));
                        shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureWeights)));
                        shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureAssemble), coarsestMipLevel));
                        shaderArguments.PushConstants(0, (float)coarsestMipLevel);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LocalExposureBlendExposures);
                        commandList.Dispatch2D(
                            computeShader,
                            shaderArguments,
                            threadGroupCountX,
                            groupCountY);
                    };
            });

        renderGraph.AddPass(std::format("LocalExposureBlendLaplacian (Compute, {}x{})", width, height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(localExposureLuminances, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(localExposureWeights, RenderBackendResourceState::ShaderResource);

                localExposureAssemble = builder.ReadWriteTexture(localExposureAssemble, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 w = coarsestMipLevelWidth;
                        uint32 h = coarsestMipLevelHeight;

                        for (int32 mipLevel = coarsestMipLevel; mipLevel > displayMipLevel; mipLevel--)
                        {
                            RenderBackendBarrier transitions[] =
                            {
                                RenderBackendBarrier(registry.GetRenderBackendTextureHandle(localExposureAssemble), RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
                            };
                            commandList.Transitions(transitions, 1);

                            w = w << 1;
                            h = h << 1;

                            uint32 threadGroupCountX = ComputeWorkGroupCount(w, PostProcessingThreadGroupSizeX);
                            uint32 threadGroupCountY = ComputeWorkGroupCount(h, PostProcessingThreadGroupSizeY);

                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                            shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureLuminances)));
                            shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureWeights)));
                            shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureAssemble)));
                            shaderArguments.BindTextureUAV(4, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureAssemble), mipLevel - 1));
                            shaderArguments.PushConstants(0, (float)mipLevel);

                            RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LocalExposureBlendLaplacian);
                            commandList.Dispatch2D(
                                computeShader,
                                shaderArguments,
                                threadGroupCountX,
                                groupCountY);
                        }

                        if ((coarsestMipLevel - displayMipLevel > 0) && (coarsestMipLevel < (int)mipLevels - 1))
                        {
                            RenderBackendBarrier transitions[] =
                            {
                                RenderBackendBarrier(registry.GetRenderBackendTextureHandle(localExposureAssemble), RenderBackendTextureSubresourceRange(displayMipLevel + 1, coarsestMipLevel - displayMipLevel, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::ShaderResource, RenderBackendResourceState::UnorderedAccess),
                            };
                            commandList.Transitions(transitions, 1);
                        }
                    };
            });

        uint32 displayMipLevelWidth = width >> displayMipLevel;
        uint32 displayMipLevelHeight = height >> displayMipLevel;

        renderGraph.AddPass(std::format("LocalExposureGuidedUpsampling (Compute, {}x{} -> {}x{})", displayMipLevelWidth, displayMipLevelHeight, targetResolution.width, targetResolution.height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(localExposureLuminances, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(localExposureAssemble, RenderBackendResourceState::ShaderResource);

                localExposureAssemble = builder.WriteTexture(localExposureAssemble, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 threadGroupCountX = ComputeWorkGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                        uint32 threadGroupCountY = ComputeWorkGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorTexture)));
                        if (isAutoExposureTextureValid) shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(autoExposureTexture)));
                        shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureLuminances)));
                        shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureAssemble)));
                        shaderArguments.BindTextureUAV(4, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(localExposureTexture), 0));
                        shaderArguments.PushConstants(0, (float)displayMipLevel);

                        shaderArguments.PushConstants(1, (float)displayMipLevelWidth);
                        shaderArguments.PushConstants(2, (float)displayMipLevelHeight);
                        shaderArguments.PushConstants(3, 1.0f / (float)displayMipLevelWidth);
                        shaderArguments.PushConstants(4, 1.0f / (float)displayMipLevelHeight);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LocalExposureGuidedUpsampling);
                        commandList.Dispatch2D(
                            computeShader,
                            shaderArguments,
                            threadGroupCountX,
                            groupCountY);
                    };
            });

        return localExposureTexture;
    }
#endif
}