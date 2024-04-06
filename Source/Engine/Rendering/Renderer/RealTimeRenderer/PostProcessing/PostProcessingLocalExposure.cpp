#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace HE
{
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

        uint32 width = std::max(1u, Math::CeilDiv(targetResolutionX, downsampleFactor));
        uint32 height = std::max(1u, Math::CeilDiv(targetResolutionY, downsampleFactor));
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
                        uint32 dispatchX = Math::CeilDiv(width, PostProcessingThreadGroupCountX);
                        uint32 dispatchY = Math::CeilDiv(height, PostProcessingThreadGroupCountY);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneColorTexture)));
                        if (isAutoExposureTextureValid) shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(autoExposureTexture)));
                        shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(localExposureLuminances), 0));
                        shaderArguments.PushConstants(0, highlights);
                        shaderArguments.PushConstants(1, shadows);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::LocalExposureComputeLuminances);
                        commandList.Dispatch2D(
                            computeShader,
                            shaderArguments,
                            dispatchX,
                            dispatchY);
                    };
            });

        renderGraph.AddPass(std::format("LocalExposureComputeWeights (Compute, {}x{})", width, height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(localExposureLuminances, RenderBackendResourceState::ShaderResource);

                localExposureWeights = builder.WriteTexture(localExposureWeights, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 dispatchX = Math::CeilDiv(width, PostProcessingThreadGroupCountX);
                        uint32 dispatchY = Math::CeilDiv(height, PostProcessingThreadGroupCountY);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(localExposureLuminances)));
                        shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(localExposureWeights), 0));
                        shaderArguments.PushConstants(0, sigma);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::LocalExposureComputeWeights);
                        commandList.Dispatch2D(
                            computeShader,
                            shaderArguments,
                            dispatchX,
                            dispatchY);
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
                        RenderBackendTextureHandle textureHandle = registry.GetRenderBackendTexture(localExposureLuminances);
                        RenderBackendShaderHandle downsampleTexture2DCS = renderEngine->GetShaderLibrary()->GetShaderHandle((uint32)ShaderPipelineID::DownsampleTexture2D);
                        for (uint32 mipLevel = 1; mipLevel < mipLevels; mipLevel++)
                        {
                            if (mipLevel == 1)
                            {
                                RenderBackendBarrier transitions[] =
                                {
                                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
                                };
                                commandList.Transitions(transitions, 1);
                            }
                            else
                            {
                                RenderBackendBarrier transitions[] =
                                {
                                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
                                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
                                };
                                commandList.Transitions(transitions, 2);
                            }

                            w = w >> 1;
                            h = h >> 1;

                            uint32 dispatchX = Math::CeilDiv(w, 8);
                            uint32 dispatchY = Math::CeilDiv(h, 8);

                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(textureHandle));
                            shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(textureHandle, mipLevel));
                            shaderArguments.PushConstants(0, (float)(mipLevel - 1));

                            commandList.Dispatch2D(
                                downsampleTexture2DCS,
                                shaderArguments,
                                dispatchX,
                                dispatchY);
                        }
                        RenderBackendBarrier transition = RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevels - 1, RENDER_BACKEND_REMAINING_MIP_LEVELS, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
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
                        RenderBackendTextureHandle textureHandle = registry.GetRenderBackendTexture(localExposureWeights);
                        RenderBackendShaderHandle downsampleTexture2DCS = renderEngine->GetShaderLibrary()->GetShaderHandle((uint32)ShaderPipelineID::DownsampleTexture2D);
                        for (uint32 mipLevel = 1; mipLevel < mipLevels; mipLevel++)
                        {
                            if (mipLevel == 1)
                            {
                                RenderBackendBarrier transitions[] =
                                {
                                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
                                };
                                commandList.Transitions(transitions, 1);
                            }
                            else
                            {
                                RenderBackendBarrier transitions[] =
                                {
                                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
                                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::Undefined, RenderBackendResourceState::UnorderedAccess)
                                };
                                commandList.Transitions(transitions, 2);
                            }

                            w = w >> 1;
                            h = h >> 1;

                            uint32 dispatchX = Math::CeilDiv(w, 8);
                            uint32 dispatchY = Math::CeilDiv(h, 8);

                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(textureHandle));
                            shaderArguments.BindTextureUAV(1, RenderBackendTextureUAVDesc::Create(textureHandle, mipLevel));
                            shaderArguments.PushConstants(0, (float)(mipLevel - 1));

                            commandList.Dispatch2D(
                                downsampleTexture2DCS,
                                shaderArguments,
                                dispatchX,
                                dispatchY);
                        }
                        RenderBackendBarrier transition = RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevels - 1, RENDER_BACKEND_REMAINING_MIP_LEVELS, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource);
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
                        uint32 dispatchX = Math::CeilDiv(coarsestMipLevelWidth, PostProcessingThreadGroupCountX);
                        uint32 dispatchY = Math::CeilDiv(coarsestMipLevelHeight, PostProcessingThreadGroupCountY);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(localExposureLuminances)));
                        shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(localExposureWeights)));
                        shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(localExposureAssemble), coarsestMipLevel));
                        shaderArguments.PushConstants(0, (float)coarsestMipLevel);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::LocalExposureBlendExposures);
                        commandList.Dispatch2D(
                            computeShader,
                            shaderArguments,
                            dispatchX,
                            dispatchY);
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
                                RenderBackendBarrier(registry.GetRenderBackendTexture(localExposureAssemble), RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::ShaderResource),
                            };
                            commandList.Transitions(transitions, 1);

                            w = w << 1;
                            h = h << 1;

                            uint32 dispatchX = Math::CeilDiv(w, PostProcessingThreadGroupCountX);
                            uint32 dispatchY = Math::CeilDiv(h, PostProcessingThreadGroupCountY);

                            RenderBackendShaderArguments shaderArguments = {};
                            shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                            shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(localExposureLuminances)));
                            shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(localExposureWeights)));
                            shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(localExposureAssemble)));
                            shaderArguments.BindTextureUAV(4, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(localExposureAssemble), mipLevel - 1));
                            shaderArguments.PushConstants(0, (float)mipLevel);

                            RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::LocalExposureBlendLaplacian);
                            commandList.Dispatch2D(
                                computeShader,
                                shaderArguments,
                                dispatchX,
                                dispatchY);
                        }

                        if ((coarsestMipLevel - displayMipLevel > 0) && (coarsestMipLevel < (int)mipLevels - 1))
                        {
                            RenderBackendBarrier transitions[] =
                            {
                                RenderBackendBarrier(registry.GetRenderBackendTexture(localExposureAssemble), RenderBackendTextureSubresourceRange(displayMipLevel + 1, coarsestMipLevel - displayMipLevel, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::ShaderResource, RenderBackendResourceState::UnorderedAccess),
                            };
                            commandList.Transitions(transitions, 1);
                        }
                    };
            });

        uint32 displayMipLevelWidth = width >> displayMipLevel;
        uint32 displayMipLevelHeight = height >> displayMipLevel;

        renderGraph.AddPass(std::format("LocalExposureGuidedUpsampling (Compute, {}x{} -> {}x{})", displayMipLevelWidth, displayMipLevelHeight, targetResolutionX, targetResolutionY), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(localExposureLuminances, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(localExposureAssemble, RenderBackendResourceState::ShaderResource);

                localExposureAssemble = builder.WriteTexture(localExposureAssemble, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 dispatchX = Math::CeilDiv(targetResolutionX, PostProcessingThreadGroupCountX);
                        uint32 dispatchY = Math::CeilDiv(targetResolutionY, PostProcessingThreadGroupCountY);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneColorTexture)));
                        if (isAutoExposureTextureValid) shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(autoExposureTexture)));
                        shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(localExposureLuminances)));
                        shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(localExposureAssemble)));
                        shaderArguments.BindTextureUAV(4, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(localExposureTexture), 0));
                        shaderArguments.PushConstants(0, (float)displayMipLevel);

                        shaderArguments.PushConstants(1, (float)displayMipLevelWidth);
                        shaderArguments.PushConstants(2, (float)displayMipLevelHeight);
                        shaderArguments.PushConstants(3, 1.0f / (float)displayMipLevelWidth);
                        shaderArguments.PushConstants(4, 1.0f / (float)displayMipLevelHeight);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::LocalExposureGuidedUpsampling);
                        commandList.Dispatch2D(
                            computeShader,
                            shaderArguments,
                            dispatchX,
                            dispatchY);
                    };
            });

        return localExposureTexture;
    }
}