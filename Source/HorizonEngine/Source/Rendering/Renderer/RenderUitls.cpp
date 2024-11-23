#include "RenderUtils.h"
#include "ShaderLibrary.h"
#include "ImageBasedLighting.h"

namespace Horizon
{
    RendererDefaultResources::RendererDefaultResources(RenderBackend* renderBackend, RenderGraphResourcePool* resourcePool, ShaderLibrary* shaderLibrary)
        : renderBackend(renderBackend)
        , renderGraphResourcePool(resourcePool)
        , shaderLibrary(shaderLibrary)
        , initialized(false)
        , blackDummyTexture2D(nullptr)
        , whiteDummyTexture2D(nullptr)
    {
        assert(!initialized);
    }

    RendererDefaultResources::~RendererDefaultResources()
    {
        assert(!initialized);
    }

    void RendererDefaultResources::Initialize(RenderBackendCommandList& commandList)
    {
        assert(!initialized);
        RenderGraphTextureDesc dummyTextureDesc = RenderGraphTextureDesc::Create2D(
            1,
            1,
            RenderBackendTextureFormat::B8G8R8A8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource);

        const uint8 blackColor[4] = { 0, 0, 0, 0 };
        RenderBackendTextureHandle blackDummyTexture2DHandle = renderBackend->CreateTexture(&dummyTextureDesc, &blackColor, "BlackDummyTexture2D");
        blackDummyTexture2D = renderGraphResourcePool->CacheTexture(blackDummyTexture2DHandle, dummyTextureDesc, "BlackDummyTexture2D");

        const uint8 whiteColor[4] = { 255, 255, 255, 255 };
        RenderBackendTextureHandle whiteDummyTexture2DHandle = renderBackend->CreateTexture(&dummyTextureDesc, &whiteColor, "WhiteDummyTexture2D");
        whiteDummyTexture2D = renderGraphResourcePool->CacheTexture(whiteDummyTexture2DHandle, dummyTextureDesc, "WhiteDummyTexture2D");

        RenderBackendTextureDesc environmentBrdfLutTextureDesc = RenderBackendTextureDesc::Create2D(
            GEnvironmentBrdfLutTextureSize,
            GEnvironmentBrdfLutTextureSize,
            RenderBackendTextureFormat::R16G16Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        environmentBrdfLutTexture = renderBackend->CreateTexture(&environmentBrdfLutTextureDesc, nullptr, "EnvironmentBrdfLut");

        // TODO: refactor this
        {
            RenderBackendSamplerDesc globalSamplerPointWarpDesc = RenderBackendSamplerDesc::CreatePointWarp(0.0f, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1);
            globalSamplerPointWarp = renderBackend->CreateSampler(&globalSamplerPointWarpDesc, "GlobalSamplerPointWarp");
            RenderBackendSamplerDesc globalSamplerPointClampDesc = RenderBackendSamplerDesc::CreatePointClamp(0.0f, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1);
            globalSamplerPointClamp = renderBackend->CreateSampler(&globalSamplerPointClampDesc, "GlobalSamplerPointClamp");
            RenderBackendSamplerDesc globalSamplerPointBorderDesc = RenderBackendSamplerDesc::CreatePointBorder(0.0f, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1);
            globalSamplerPointBorder = renderBackend->CreateSampler(&globalSamplerPointBorderDesc, "GlobalSamplerPointBorder");

            RenderBackendSamplerDesc globalSamplerLinearWarpDesc = RenderBackendSamplerDesc::CreateLinearWarp(0.0f, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1);
            globalSamplerLinearWarp = renderBackend->CreateSampler(&globalSamplerLinearWarpDesc, "GlobalSamplerLinearWarp");
            RenderBackendSamplerDesc globalSamplerLinearClampDesc = RenderBackendSamplerDesc::CreateLinearClamp(0.0f, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1);
            globalSamplerLinearClamp = renderBackend->CreateSampler(&globalSamplerLinearClampDesc, "GlobalSamplerLinearClamp");
            RenderBackendSamplerDesc globalSamplerLinearBorderDesc = RenderBackendSamplerDesc::CreateLinearBorder(0.0f, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1);
            globalSamplerLinearBorder = renderBackend->CreateSampler(&globalSamplerLinearBorderDesc, "GlobalSamplerLinearBorder");

            RenderBackendSamplerDesc globalSamplerComparisonGreaterLinearClampDesc = RenderBackendSamplerDesc::CreateComparisonLinearClamp(0.0f, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1, RenderBackendCompareOp::Greater);
            globalSamplerComparisonGreaterLinearClamp = renderBackend->CreateSampler(&globalSamplerComparisonGreaterLinearClampDesc, "GlobalSamplerComparisonGreaterLinearClamp");
            RenderBackendSamplerDesc globalSamplerComparisonLessLinearClampDesc = RenderBackendSamplerDesc::CreateComparisonLinearClamp(0.0f, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1, RenderBackendCompareOp::Less);
            globalSamplerComparisonLessLinearClamp = renderBackend->CreateSampler(&globalSamplerComparisonLessLinearClampDesc, "GlobalSamplerComparisonLessLinearClamp");
        }

        RenderEnvironmentBrdfLut(renderBackend, shaderLibrary, commandList, environmentBrdfLutTexture);

        initialized = true;
    }

    void RendererDefaultResources::Release()
    {
        assert(initialized);
        renderBackend->FlushRenderDevices();

        renderBackend->DestroyTexture(environmentBrdfLutTexture);

        initialized = false;
    }

    RenderBackendTextureHandle RendererDefaultResources::GetEnvironmentBrdfLutTexture() const
    {
        return environmentBrdfLutTexture;
    }

    RenderGraphPersistentTexture* RendererDefaultResources::GetBlackDummyTexture2D() const
    {
        return blackDummyTexture2D;
    }

    RenderGraphPersistentTexture* RendererDefaultResources::GetWhiteDummyTexture2D() const
    {
        return whiteDummyTexture2D;
    }

    RenderGraphTextureHandle RendererDefaultResources::ImportWhiteDummyTexture2D(RenderGraph& renderGraph) const
    {
        return renderGraph.ImportExternalTexture(whiteDummyTexture2D, "WhiteDummyTexture2D");
    }

    RenderGraphTextureHandle RendererDefaultResources::ImportBlackDummyTexture2D(RenderGraph& renderGraph) const
    {
        return renderGraph.ImportExternalTexture(blackDummyTexture2D, "BlackDummyTexture2D");
    }

    void Texture2DGenerateMips(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle textureHandle, uint32 width, uint32 height, uint32 numMipLevels)
    {
        if (numMipLevels < 2)
        {
            return;
        }

        RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::DownsampleTexture2DVS);
        RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::DownsampleTexture2DPS);

        for (uint32 mipLevel = 1; mipLevel < numMipLevels; mipLevel++)
        {
            width = width >> 1;
            height = height >> 1;

            RenderBackendViewport viewport(0.0f, 0.0f, float(width), float(height));
            commandList.SetViewports(&viewport, 1);

            RenderBackendScissor scissor(0, 0, width, height);
            commandList.SetScissors(&scissor, 1);

            if (mipLevel == 1)
            {
                RenderBackendBarrier transitions[] =
                {
                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::RenderTarget)
                };
                commandList.Transitions(transitions, 1);
            }
            else
            {
                RenderBackendBarrier transitions[] =
                {
                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::RenderTarget, RenderBackendResourceState::ShaderResource),
                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::Undefined, RenderBackendResourceState::RenderTarget)
                };
                commandList.Transitions(transitions, 2);
            }

            RenderBackendRenderPassInfo renderPass =
            {
                .renderTargets =
                {
                    {
                        .texture = textureHandle,
                        .mipLevel = mipLevel,
                        .arrayLayer = 0,
                        .loadOp = RenderBackendRenderPassBeginningAccessType::Discard,
                        .storeOp = RenderBackendRenderPassEndingAccessType::Preserve
                    }
                },
            };

            commandList.BeginRenderPass(renderPass);

            RenderBackendShaderConstants shaderConstants = {};
            shaderConstants.BindTextureSRV(0, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(textureHandle));
            shaderConstants.BindScalar(2, mipLevel - 1u);
            shaderConstants.BindScalar(3, float(width));
            shaderConstants.BindScalar(4, float(height));

            RenderBackendGraphicsPipelineState graphicsPipelineState = {};

            commandList.Draw(
                vertexShader,
                pixelShader,
                graphicsPipelineState,
                shaderConstants,
                3, 1, 0, 0,
                RenderBackendPrimitiveTopology::TriangleList);

            commandList.EndRenderPass();

            //uint32 threadGroupCountX = ComputeWorkGroupCount(width, 8);
            //uint32 threadGroupCountY = ComputeWorkGroupCount(height, 8);
            //uint32 threadGroupCountZ = 1;

            //commandList.Dispatch(
            //    downsampleTexture2DCS,
            //    shaderConstants,
            //    threadGroupCountX,
            //    threadGroupCountY,
            //    threadGroupCountZ);
        }
        RenderBackendBarrier transition(textureHandle, RenderBackendTextureSubresourceRange(numMipLevels - 1, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers), RenderBackendResourceState::RenderTarget, RenderBackendResourceState::ShaderResource);
        commandList.Transitions(&transition, 1);
    }
}