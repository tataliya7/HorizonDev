#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    int ringCount = 2;
    int ringSampleFactor = 8;

    int octawebRingSampleCount(int ring)
    {
        return ring == 0 ? 1 : ringSampleFactor * ring;
    }

    Vector2 OctawebSample(int ring, int s, int ringSampleCount, int ringCount)
    {
        // Need to place n-1 rings along the radius since center is ring 0
        float ringSpacing = 1.0f / float(ringCount - 1);
        float r = float(ring) * ringSpacing;

        float rimSpacing = 2.0f * glm::pi<float>() / float(ringSampleCount);
        float phi = float(s) * rimSpacing;
        // Rings are 'interleaved', probably to make the pattern less obviously repeating
        if (ring % 2 == 0)
            phi += rimSpacing * 0.5f;

        float x = r * cos(phi);
        float y = r * sin(phi);

        return Vector2(x, y);
    }

    void GenerateOctawebSamples()
    {
        for (int ring = 0; ring < ringCount; ++ring)
        {
            int ringSampleCount = octawebRingSampleCount(ring);
            for (int si = 0; si < ringSampleCount; ++si)
            {
                Vector2 sample = OctawebSample(ring, si, ringSampleCount, ringCount);
                printf("float2(%.06f, %.06f)\n", sample.x, sample.y);
            }
        }
    }

    RenderGraphTextureHandle RealTimeRenderer::AddDepthOfFieldPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture)
    {
        // GenerateOctawebSamples();

        const uint32 downsampleFactor = 2;
        uint32 dofWidth = view.targetWidth / downsampleFactor;
        uint32 dofHeight = view.targetHeight / downsampleFactor;

        RenderGraphTextureDesc dofSceneColorInputTextureDesc = RenderGraphTextureDesc::Create2D(
            dofWidth,
            dofHeight,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle sceneColorInputTexture = renderGraph.CreateTexture(dofSceneColorInputTextureDesc, "DOFSceneColorInputTexture");

        sceneColorInputTexture = AddDownsamplePass(renderGraph, view, view.targetWidth, view.targetHeight, dofSceneColorInputTextureDesc.width, dofSceneColorInputTextureDesc.height, sceneColorTexture, sceneColorInputTexture);

        RenderGraphTextureDesc depthOfFieldCoCTextureDesc = RenderGraphTextureDesc::Create2D(
            view.targetWidth,
            view.targetHeight,
            RenderBackendTextureFormat::R16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle depthOfFieldCoCTexture = renderGraph.CreateTexture(depthOfFieldCoCTextureDesc, "DepthOfFieldCoCTextureDesc");

        RenderGraphTextureDesc depthOfFieldTextureDesc = RenderGraphTextureDesc::Create2D(
            dofWidth,
            dofHeight,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle depthOfFieldGatherTexture = renderGraph.CreateTexture(depthOfFieldTextureDesc, "DepthOfFieldGatherTexture");
        RenderGraphTextureHandle depthOfFieldPostfilterTexture = renderGraph.CreateTexture(depthOfFieldTextureDesc, "DepthOfFieldPostfilterTexture");

        RenderGraphTextureDesc depthOfFieldOutputTextureDesc = RenderGraphTextureDesc::Create2D(
            view.targetWidth,
            view.targetHeight,
            RenderBackendTextureFormat::RGBA16Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::RenderTarget,
            RenderBackendTextureClearValue::None,
            1,
            1,
            RenderBackendResourceState::UnorderedAccess);
        RenderGraphTextureHandle depthOfFieldOutputTexture = renderGraph.CreateTexture(depthOfFieldOutputTextureDesc, "DepthOfFieldOutputTexture");

        renderGraph.AddPass("DepthOfFieldSetup", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                const auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);

                depthOfFieldCoCTexture = builder.WriteTexture(depthOfFieldCoCTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(view.targetWidth, PostProcessingThreadGroupCountX);
                    uint32 groupCountY = ComputeWorkGroupCount(view.targetHeight, PostProcessingThreadGroupCountY);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(depthOfFieldCoCTexture), 0));

                    shaderArguments.PushConstants(0, settings.postProcessingSettings.dofFocalDistance);
                    shaderArguments.PushConstants(1, settings.postProcessingSettings.dofFocalRegion);
                    shaderArguments.PushConstants(2, settings.postProcessingSettings.dofNearTransitionRegion);
                    shaderArguments.PushConstants(3, settings.postProcessingSettings.dofFarTransitionRegion);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::DepthOfFieldSetup);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
                };
            });

        renderGraph.AddPass("DepthOfFieldGather", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(sceneColorInputTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(depthOfFieldCoCTexture, RenderBackendResourceState::ShaderResource);

                depthOfFieldGatherTexture = builder.WriteTexture(depthOfFieldGatherTexture, RenderBackendResourceState::ShaderResource);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(dofWidth, PostProcessingThreadGroupCountX);
                    uint32 groupCountY = ComputeWorkGroupCount(dofHeight, PostProcessingThreadGroupCountY);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorInputTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(depthOfFieldCoCTexture)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(depthOfFieldGatherTexture), 0));

                    shaderArguments.PushConstants(0, settings.postProcessingSettings.dofFarRegionBlurSize);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::DepthOfFieldGather);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
                };
            });

        renderGraph.AddPass("DepthOfFieldPostfilter", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(depthOfFieldGatherTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(depthOfFieldCoCTexture, RenderBackendResourceState::ShaderResource);

                depthOfFieldPostfilterTexture = builder.WriteTexture(depthOfFieldPostfilterTexture, RenderBackendResourceState::ShaderResource);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(dofWidth, PostProcessingThreadGroupCountX);
                    uint32 groupCountY = ComputeWorkGroupCount(dofHeight, PostProcessingThreadGroupCountY);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(depthOfFieldGatherTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(depthOfFieldCoCTexture)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(depthOfFieldPostfilterTexture), 0));

                    shaderArguments.PushConstants(0, settings.postProcessingSettings.dofFarRegionBlurSize);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::DepthOfFieldPostfilter);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
                };
            });

        renderGraph.AddPass("DepthOfFieldRecombine", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(sceneColorTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(depthOfFieldPostfilterTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(depthOfFieldCoCTexture, RenderBackendResourceState::ShaderResource);

                depthOfFieldOutputTexture = builder.WriteTexture(depthOfFieldOutputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(view.targetWidth, PostProcessingThreadGroupCountX);
                    uint32 groupCountY = ComputeWorkGroupCount(view.targetHeight, PostProcessingThreadGroupCountY);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, sceneViewShaderParametersBuffer, 0);
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(depthOfFieldPostfilterTexture)));
                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(depthOfFieldCoCTexture)));
                    shaderArguments.BindTextureUAV(4, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(depthOfFieldOutputTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShaderHandle(ShaderID::DepthOfFieldRecombine);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
                };
            });

        return depthOfFieldOutputTexture;
    }
}