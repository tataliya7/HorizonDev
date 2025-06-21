#include "../RealTimeRenderer.h"
#include "PostProcessing.h"

namespace Horizon
{
    int ringCount = 2;
    int ringSampleFactor = 8;

    int octawebRingSampleCount(int ring)
    {
        return ring == 0 ? 1 : ringSampleFactor * ring;
    }

    Vector2f OctawebSample(int ring, int s, int ringSampleCount, int ringCount)
    {
        // Need to place n-1 rings along the radius since position is ring 0
        float ringSpacing = 1.0f / float(ringCount - 1);
        float r = float(ring) * ringSpacing;

        float rimSpacing = 2.0f * glm::pi<float>() / float(ringSampleCount);
        float phi = float(s) * rimSpacing;
        // Rings are 'interleaved', probably to make the pattern less obviously repeating
        if (ring % 2 == 0)
            phi += rimSpacing * 0.5f;

        float x = r * cos(phi);
        float y = r * sin(phi);

        return Vector2f(x, y);
    }

    void GenerateOctawebSamples()
    {
        for (int ring = 0; ring < ringCount; ++ring)
        {
            int ringSampleCount = octawebRingSampleCount(ring);
            for (int si = 0; si < ringSampleCount; ++si)
            {
                Vector2f sample = OctawebSample(ring, si, ringSampleCount, ringCount);
                printf("float2(%.06f, %.06f)\n", sample.x, sample.y);
            }
        }
    }

    bool RealTimeRenderer::IsDepthOfFieldEnabled() const
    {
        return features.enableDepthOfField;
    }


    RenderGraphTextureHandle RealTimeRenderer::DispatchDepthOfField(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture)
    {
        return RenderGraphTextureHandle::Null;
    }
#if 0
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


                {
                    uint32 threadGroupCountX = ComputeWorkGroupCount(view.targetWidth, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeWorkGroupCount(view.targetHeight, PostProcessingThreadGroupSizeY);

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneColorTexture)));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture)));
                    shaderConstants.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndexdepthOfFieldCoCTexture), 0));

                    shaderConstants.PushConstants(0, settings.postProcessingSettings.dofFocalDistance);
                    shaderConstants.PushConstants(1, settings.postProcessingSettings.dofFocalRegion);
                    shaderConstants.PushConstants(2, settings.postProcessingSettings.dofNearTransitionRegion);
                    shaderConstants.PushConstants(3, settings.postProcessingSettings.dofFarTransitionRegion);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::DepthOfFieldSetup);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        groupCountY);
                };
            });

        renderGraph.AddPass("DepthOfFieldGather", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(sceneColorInputTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(depthOfFieldCoCTexture, RenderBackendResourceState::ShaderResource);

                depthOfFieldGatherTexture = builder.WriteTexture(depthOfFieldGatherTexture, RenderBackendResourceState::ShaderResource);


                {
                    uint32 threadGroupCountX = ComputeWorkGroupCount(dofWidth, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeWorkGroupCount(dofHeight, PostProcessingThreadGroupSizeY);

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneColorInputTexture)));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(depthOfFieldCoCTexture)));
                    shaderConstants.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndexdepthOfFieldGatherTexture), 0));

                    shaderConstants.PushConstants(0, settings.postProcessingSettings.dofFarRegionBlurSize);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::DepthOfFieldGather);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        groupCountY);
                };
            });

        renderGraph.AddPass("DepthOfFieldPostfilter", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.ReadTexture(depthOfFieldGatherTexture, RenderBackendResourceState::ShaderResource);
                builder.ReadTexture(depthOfFieldCoCTexture, RenderBackendResourceState::ShaderResource);

                depthOfFieldPostfilterTexture = builder.WriteTexture(depthOfFieldPostfilterTexture, RenderBackendResourceState::ShaderResource);


                {
                    uint32 threadGroupCountX = ComputeWorkGroupCount(dofWidth, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeWorkGroupCount(dofHeight, PostProcessingThreadGroupSizeY);

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(depthOfFieldGatherTexture)));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(depthOfFieldCoCTexture)));
                    shaderConstants.BindTextureUAV(3, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndexdepthOfFieldPostfilterTexture), 0));

                    shaderConstants.PushConstants(0, settings.postProcessingSettings.dofFarRegionBlurSize);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::DepthOfFieldPostfilter);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
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


                {
                    uint32 threadGroupCountX = ComputeWorkGroupCount(view.targetWidth, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeWorkGroupCount(view.targetHeight, PostProcessingThreadGroupSizeY);

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneColorTexture)));
                    shaderConstants.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(depthOfFieldPostfilterTexture)));
                    shaderConstants.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(depthOfFieldCoCTexture)));
                    shaderConstants.BindTextureUAV(4, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndexdepthOfFieldOutputTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::DepthOfFieldRecombine);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderConstants,
                        threadGroupCountX,
                        groupCountY);
                };
            });

        return depthOfFieldOutputTexture;
    }
#endif
}