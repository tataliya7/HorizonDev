#include "RealTimeRenderer.h"

namespace HE
{
    void RealTimeRenderer::RenderLightShafts(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        return;

        Vector2 lightShaftsCenter;
        for (uint32 lightIndex = 0; lightIndex < renderEngine->numLights; lightIndex++)
        {
            if (renderEngine->lightData[lightIndex].type == (uint32)LightComponent::LightType::Directional)
            {
                const LightComponent* light = renderEngine->lightInfo[lightIndex].component;
                Vector3 dir = light->GetDirection();
                dir = Vector3(sceneViewShaderParameters.atmosphereLightDirection.x, sceneViewShaderParameters.atmosphereLightDirection.y, sceneViewShaderParameters.atmosphereLightDirection.z);
                Vector3 lightShaftsPostion = sceneViewShaderParameters.cameraPosition - dir;
                Vector4 clippos = sceneViewShaderParameters.viewProjectionMatrix * Vector4(lightShaftsPostion.x, lightShaftsPostion.y, lightShaftsPostion.z, 1.0f);
                lightShaftsCenter = Vector2(clippos.x / clippos.w, clippos.y / clippos.w);
                lightShaftsCenter = lightShaftsCenter * Vector2(0.5f, -0.5f) + 0.5f;
                break;
            }
        }

        uint32 lightShaftsDownsampleFactor = 2;

        Vector2u lightShaftsTextureSize;
        lightShaftsTextureSize.x = Math::CeilDiv(renderResolutionX, lightShaftsDownsampleFactor);
        lightShaftsTextureSize.y = Math::CeilDiv(renderResolutionY, lightShaftsDownsampleFactor);

        Vector2 aspectRatioAndInvAspectRatio = Vector2((float)lightShaftsTextureSize.x / (float)lightShaftsTextureSize.y, (float)lightShaftsTextureSize.y / (float)lightShaftsTextureSize.x);

        RenderGraphTextureDesc lightShaftsTextureDesc = RenderGraphTextureDesc::Create2D(
            lightShaftsTextureSize.x,
            lightShaftsTextureSize.y,
            RenderBackendTextureFormat::R11G11B10Float,
            RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
        RenderGraphTextureHandle lightShaftsDownsampleOutputTexture = renderGraph.CreateTexture(lightShaftsTextureDesc, "LightShaftsDownsampleOutputTexture");

        renderGraph.AddPass(std::format("LightShaftsDownsample (Compute, {}x{})", lightShaftsTextureSize.x, lightShaftsTextureSize.y), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                auto sceneColorTexture = builder.ReadTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::ShaderResource);
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);

                lightShaftsDownsampleOutputTexture = builder.WriteTexture(lightShaftsDownsampleOutputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 dispatchX = Math::CeilDiv(lightShaftsTextureSize.x, 8);
                    uint32 dispatchY = Math::CeilDiv(lightShaftsTextureSize.y, 8);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneColorTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneDepthTexture)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(lightShaftsDownsampleOutputTexture), 0));

                    shaderArguments.PushConstants(0, (float)lightShaftsTextureSize.x);
                    shaderArguments.PushConstants(1, (float)lightShaftsTextureSize.y);
                    shaderArguments.PushConstants(2, lightShaftsCenter.x);
                    shaderArguments.PushConstants(3, lightShaftsCenter.y);
                    shaderArguments.PushConstants(4, aspectRatioAndInvAspectRatio.x);
                    shaderArguments.PushConstants(5, aspectRatioAndInvAspectRatio.y);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::LightShaftsDownsample);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        dispatchX,
                        dispatchY);
                };
            });

        if (true)
        {

        }

        RenderGraphTextureHandle radialBlurTexture = lightShaftsDownsampleOutputTexture;
        for (uint32 blurPassIndex = 0; blurPassIndex < 3; blurPassIndex++)
        {
            RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(lightShaftsTextureDesc, "LightShaftsRadialBlurOutputTexture");
            renderGraph.AddPass(std::format("LightShaftsRadialBlur (Compute, {}x{})", lightShaftsTextureSize.x, lightShaftsTextureSize.y), RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                    builder.ReadTexture(radialBlurTexture, RenderBackendResourceState::ShaderResource);

                    outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 dispatchX = Math::CeilDiv(lightShaftsTextureSize.x, 8);
                        uint32 dispatchY = Math::CeilDiv(lightShaftsTextureSize.y, 8);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(radialBlurTexture)));
                        shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(outputTexture), 0));
                        shaderArguments.PushConstants(2, lightShaftsCenter.x);
                        shaderArguments.PushConstants(3, lightShaftsCenter.y);
                        shaderArguments.PushConstants(4, (float)blurPassIndex);

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::LightShaftsRadialBlur);
                        commandList.Dispatch2D(
                            computeShader,
                            shaderArguments,
                            dispatchX,
                            dispatchY);
                    };
                });
            radialBlurTexture = outputTexture;
        }

        RenderGraphTextureHandle lightShaftsApplyTexture = renderGraph.CreateTexture(historySceneColorTextureCache.desc, "LightShaftsApplyTexture");

        // TODO: Read write UAV ?
        renderGraph.AddPass(std::format("LightShaftsApply (Compute, {}x{})", renderResolutionX, renderResolutionY), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                auto sceneColorTexture = builder.ReadTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(radialBlurTexture, RenderBackendResourceState::ShaderResource);

                lightShaftsApplyTexture = sceneTextures.sceneColorTexture = builder.WriteTexture(lightShaftsApplyTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 dispatchX = Math::CeilDiv(renderResolutionX, 8);
                    uint32 dispatchY = Math::CeilDiv(renderResolutionY, 8);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneColorTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(radialBlurTexture)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(lightShaftsApplyTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::LightShaftsApply);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        dispatchX,
                        dispatchY);
                };
            });
    }
}