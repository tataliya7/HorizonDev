#include "RealTimeRenderer.h"

namespace Horizon
{
    static constexpr uint32 GScreenSpaceLightShaftsDownsampleFactor = 2;
    static constexpr uint32 GScreenSpaceLightShaftsRadialBlurPassCount = 3;

    bool RealTimeRenderer::IsScreenSpaceLightShaftsEnabled() const
    {
        return features.enableScreenSpaceLightShafts;
    }

    void RealTimeRenderer::RenderScreenSpaceLightShafts(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        return;

        // Vector2 lightShaftsCenter;
        // for (uint32 lightIndex = 0; lightIndex < renderEngine->numLights; lightIndex++)
        // {
        //     if (renderEngine->lightData[lightIndex].type == (uint32)LightComponent::LightType::Directional)
        //     {
        //         const LightComponent* light = renderEngine->lightInfo[lightIndex].component;
        //         Vector3 dir = light->GetDirection();
        //         dir = Vector3(sceneViewShaderParameters.atmosphericLightDirection.width, sceneViewShaderParameters.atmosphericLightDirection.height, sceneViewShaderParameters.atmosphericLightDirection.z);
        //         Vector3 lightShaftsPostion = sceneViewShaderParameters.cameraPosition - dir;
        //         Vector4 clippos = sceneViewShaderParameters.viewProjectionMatrix * Vector4(lightShaftsPostion.width, lightShaftsPostion.height, lightShaftsPostion.z, 1.0f);
        //         lightShaftsCenter = Vector2(clippos.width / clippos.w, clippos.height / clippos.w);
        //         lightShaftsCenter = lightShaftsCenter * Vector2(0.5f, -0.5f) + 0.5f;
        //         break;
        //     }
        // }
        //
        // Extent2D lightShaftsTextureSize = ComputeDownsampledExtent2D(renderResolution, ScreenSpaceLightShaftsDownsampleFactor);
        //
        // Vector2 aspectRatio = Vector2((float)lightShaftsTextureSize.width / (float)lightShaftsTextureSize.height, (float)lightShaftsTextureSize.height / (float)lightShaftsTextureSize.width);
        //
        // RenderGraphTextureDesc lightShaftsTextureDesc = RenderGraphTextureDesc::Create2D(
        //     lightShaftsTextureSize.width,
        //     lightShaftsTextureSize.height,
        //     RenderBackendTextureFormat::R11G11B10Float,
        //     RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        // RenderGraphTextureHandle lightShaftsDownsampleOutputTexture = renderGraph.CreateTexture(lightShaftsTextureDesc, "LightShaftsDownsampleOutputTexture");
        //
        // RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        //
        // renderGraph.AddPass(std::format("LightShaftsDownsample (Compute, {}x{})", lightShaftsTextureSize.width, lightShaftsTextureSize.height), RenderGraphPassFlags::Compute,
        //     [&](RenderGraphBuilder& builder)
        //     {
        //         RenderGraphTextureHandle sceneColorTexture = builder.ReadTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::ShaderResource);
        //         RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
        //
        //         lightShaftsDownsampleOutputTexture = builder.WriteTexture(lightShaftsDownsampleOutputTexture, RenderBackendResourceState::UnorderedAccess);
        //
        //         return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //         {
        //             uint32 threadGroupCountX = ComputeWorkGroupCount(lightShaftsTextureSize.width, 8);
        //             uint32 threadGroupCountY = ComputeWorkGroupCount(lightShaftsTextureSize.height, 8);
        //             uint32 threadGroupCountZ = 1;
        //
        //             RenderBackendShaderArguments shaderArguments = {};
        //             shaderArguments.BindBufferCBV(0, this->GetCurrentPerFrameConstantBuffer());
        //             shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorTexture)));
        //             shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
        //             shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(lightShaftsDownsampleOutputTexture), 0));
        //
        //             shaderArguments.PushConstants(0, lightShaftsCenter.x);
        //             shaderArguments.PushConstants(1, lightShaftsCenter.y);
        //             shaderArguments.PushConstants(2, aspectRatio.x);
        //             shaderArguments.PushConstants(3, aspectRatio.y);
        //
        //             RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LightShaftsDownsample);
        //             commandList.Dispatch(
        //                 computeShader,
        //                 shaderArguments,
        //                 threadGroupCountX,
        //                 threadGroupCountY,
        //                 threadGroupCountZ);
        //         };
        //     });
        //
        // // TAA
        // if (true)
        // {
        //
        // }
        //
        // RenderGraphTextureHandle radialBlurInputTexture = lightShaftsDownsampleOutputTexture;
        // RenderGraphTextureHandle radialBlurOutputTexture = radialBlurInputTexture;
        //
        // for (uint32 blurPassIndex = 0; blurPassIndex < ScreenSpaceLightShaftsRadialBlurPassCount; blurPassIndex++)
        // {
        //     radialBlurOutputTexture = renderGraph.CreateTexture(lightShaftsTextureDesc, "LightShaftsRadialBlurOutputTexture");
        //
        //     renderGraph.AddPass(std::format("LightShaftsRadialBlur (Compute, {}x{}, PassIndex={})", lightShaftsTextureSize.width, lightShaftsTextureSize.height, blurPassIndex), RenderGraphPassFlags::Compute,
        //         [&](RenderGraphBuilder& builder)
        //         {
        //             builder.ReadTexture(radialBlurInputTexture, RenderBackendResourceState::ShaderResource);
        //             radialBlurOutputTexture = builder.WriteTexture(radialBlurOutputTexture, RenderBackendResourceState::UnorderedAccess);
        //
        //             return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //                 {
        //                     uint32 threadGroupCountX = ComputeWorkGroupCount(lightShaftsTextureSize.width, 8);
        //                     uint32 threadGroupCountY = ComputeWorkGroupCount(lightShaftsTextureSize.height, 8);
        //                     uint32 threadGroupCountZ = 1;
        //
        //                     RenderBackendShaderArguments shaderArguments = {};
        //                     shaderArguments.BindBufferCBV(0, this->GetCurrentPerFrameConstantBuffer());
        //                     shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(radialBlurInputTexture)));
        //                     shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(radialBlurOutputTexture), 0));
        //                     shaderArguments.PushConstants(2, lightShaftsCenter.width);
        //                     shaderArguments.PushConstants(3, lightShaftsCenter.height);
        //                     shaderArguments.PushConstants(4, (float)blurPassIndex);
        //
        //                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::LightShaftsRadialBlur);
        //
        //                     commandList.Dispatch(
        //                         computeShader,
        //                         shaderArguments,
        //                         threadGroupCountX,
        //                         threadGroupCountY,
        //                         threadGroupCountZ);
        //                 };
        //         });
        //
        //     radialBlurInputTexture = radialBlurOutputTexture;
        // }
        //
        // renderGraph.AddPass(std::format("LightShaftsApply (Compute, {}x{})", renderResolution.width, renderResolution.height), RenderGraphPassFlags::Graphics,
        //     [&](RenderGraphBuilder& builder)
        //     {
        //         RenderGraphTextureHandle lightShaftsTexture = builder.ReadTexture(radialBlurOutputTexture, RenderBackendResourceState::ShaderResource);
        //         RenderGraphTextureHandle sceneColorTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::RenderTarget);
        //
        //         builder.BindColorTarget(0, sceneColorTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);
        //
        //         return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //             {
        //                 RenderBackendGraphicsPipelineState graphicsPipelineState = {};
        //                 graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
        //                 graphicsPipelineState.depthStencilState.depthTestEnable = false;
        //                 graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
        //                 graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::One;
        //                 graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::One;
        //                 graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
        //                 graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::RGB;
        //
        //                 RenderBackendShaderArguments shaderArguments = {};
        //                 shaderArguments.BindBufferCBV(0, this->GetCurrentPerFrameConstantBuffer());
        //                 shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(lightShaftsTexture)));
        //
        //                 RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShader(ShaderID::LightShaftsApply);
        //
        //                 commandList.Draw(
        //                     graphicsShader,
        //                     graphicsPipelineState,
        //                     shaderArguments,
        //                     3, 1, 0, 0,
        //                     RenderBackendPrimitiveTopology::TriangleList);
        //             };
        //     });
    }
}