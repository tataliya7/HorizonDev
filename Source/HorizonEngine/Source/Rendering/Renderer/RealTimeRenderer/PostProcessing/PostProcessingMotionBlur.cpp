#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    static constexpr uint32 GMotionBlurTileCount = 16;

    bool RealTimeRenderer::IsMotionBlurEnabled() const
    {
        return features.enableMotionBlur;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddMotionBlurPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        uint32 velocityTileCountX = ComputeThreadGroupCount(renderResolution.width, GMotionBlurTileCount);
        uint32 velocityTileCountY = ComputeThreadGroupCount(renderResolution.height, GMotionBlurTileCount);

        RenderGraphTextureDesc velocityAndDepthTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle velocityAndDepthTexture = renderGraph.CreateTexture(velocityAndDepthTextureDesc, "MotionBlurVelocityAndDepthTexture");

        RenderGraphTextureDesc velocityTileTextureDesc = RenderGraphTextureDesc::Create2D(
            velocityTileCountX,
            velocityTileCountY,
            RenderBackendTextureFormat::R16G16B16A16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle velocityTileTexture = renderGraph.CreateTexture(velocityTileTextureDesc, "MotionBlurVelocityTileTexture");
        RenderGraphTextureHandle dilatedVelocityTileTexture = renderGraph.CreateTexture(velocityTileTextureDesc, "MotionBlurDilatedVelocityTileTexture");

        RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget);
        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "MotionBlurSceneColorTexture");

        renderGraph.AddPass(
            std::format("MotionBlurSetup (Compute, {}x{})", renderResolution.width, renderResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle motionVectorTexture = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                velocityTileTexture = builder.WriteTexture(velocityTileTexture, RenderBackendResourceState::UnorderedAccess);
                velocityAndDepthTexture = builder.ReadTexture(velocityAndDepthTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = velocityTileCountX;
                    uint32 threadGroupCountY = velocityTileCountY;
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBufferCBV(0, this->GetCurrentPerFrameConstantBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(motionVectorTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(velocityTileTexture)));
                    shaderArguments.BindTextureUAV(4, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(velocityAndDepthTexture)));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::MotionBlurSetupCS);

                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }
}