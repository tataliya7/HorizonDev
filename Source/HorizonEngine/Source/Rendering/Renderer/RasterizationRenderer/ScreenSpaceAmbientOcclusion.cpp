#include "RasterizationRenderer.h"

namespace Horizon
{
    RenderGraphTextureHandle RasterizationRenderer::RenderScreenSpaceAmbientOcclusion(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        RenderGraphDebugLabelRegion debugLabelRegion(renderGraph, "ScreenSpaceAmbientOcclusion");

        RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        const float strength = rendererSettings.groundTruthAmbientOcclusionSettings.strength;
        const float radius = rendererSettings.groundTruthAmbientOcclusionSettings.radius;
        const float thickness = rendererSettings.groundTruthAmbientOcclusionSettings.thickness;

        // @todo Make it a configurable parameter.
        uint32 downsampleFactor = 2;
        RenderBackendTextureFormat ambientOcclusionTextureFormat = RenderBackendTextureFormat::R8Unorm;

        uint32 intermediateTextureWidth = std::max(1u, Math::CeilDiv(renderResolution.width, downsampleFactor));
        uint32 intermediateTextureHeight = std::max(1u, Math::CeilDiv(renderResolution.height, downsampleFactor));

        RenderGraphTextureDescription horizonSearchIntegralOutputTextureDescription = RenderGraphTextureDescription::Create2D(
            intermediateTextureWidth,
            intermediateTextureHeight,
            ambientOcclusionTextureFormat,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle horizonSearchIntegralOutputTexture = renderGraph.CreateTexture(horizonSearchIntegralOutputTextureDescription, "GTAOHorizonSearchIntegralOutputTexture");

        RenderGraphTextureDescription spatialFilteringOutputTextureDescription = RenderGraphTextureDescription::Create2D(
            intermediateTextureWidth,
            intermediateTextureHeight,
            ambientOcclusionTextureFormat,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle spatialFilteringOutputTexture = renderGraph.CreateTexture(spatialFilteringOutputTextureDescription, "GTAOSpatialFilteringOutputTexture");

        renderGraph.AddPass(
            std::format("GTAOHorizonSearchIntegral (Compute, {}x{})", horizonSearchIntegralOutputTextureDescription.width, horizonSearchIntegralOutputTextureDescription.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, intermediateResources.depthTexture);
                builder.SetBindlessResourceSRV(2, intermediateResources.gbuffer0);
                builder.SetBindlessResourceUAV(3, horizonSearchIntegralOutputTexture, 0);
                builder.SetShaderConstantValue(4, strength);
                builder.SetShaderConstantValue(5, radius);
                builder.SetShaderConstantValue(6, thickness);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::GTAOHorizonSearchIntegral);

                uint32 threadGroupCountX = Math::CeilDiv(horizonSearchIntegralOutputTextureDescription.width, 8);
                uint32 threadGroupCountY = Math::CeilDiv(horizonSearchIntegralOutputTextureDescription.height, 8);
                uint32 threadGroupCountZ = 1;

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        renderGraph.AddPass(
            std::format("GTAOSpatialFiltering (Compute, {}x{})", spatialFilteringOutputTextureDescription.width, spatialFilteringOutputTextureDescription.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, intermediateResources.depthTexture);
                builder.SetBindlessResourceSRV(2, horizonSearchIntegralOutputTexture);
                builder.SetBindlessResourceUAV(3, spatialFilteringOutputTexture, 0);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::GTAOSpatialFiltering);

                uint32 threadGroupCountX = Math::CeilDiv(spatialFilteringOutputTextureDescription.width, 8);
                uint32 threadGroupCountY = Math::CeilDiv(spatialFilteringOutputTextureDescription.height, 8);
                uint32 threadGroupCountZ = 1;

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        RenderGraphTextureHandle ambientOcclusionTexture = spatialFilteringOutputTexture;

        {
            RenderGraphTextureHandle temporalFilteringInputTexture = ambientOcclusionTexture;
            RenderGraphTextureHandle temporalFilteringHistoryTexture = renderGraph.ImportExternalTexture(historyFrame.ambientOcclusionTexture, "PreviousAmbientOcclusionTexture");
            if (temporalFilteringHistoryTexture.IsNull())
            {
                temporalFilteringHistoryTexture = defaultResources->ImportWhiteDummyTexture2D(renderGraph);
            }

            RenderGraphTextureDescription temporalFilteringOutputTextureDescription = RenderGraphTextureDescription::Create2D(
                intermediateTextureWidth,
                intermediateTextureHeight,
                ambientOcclusionTextureFormat,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
            RenderGraphTextureHandle temporalFilteringOutputTexture = renderGraph.CreateTexture(temporalFilteringOutputTextureDescription, "GTAOTemporalFilteringOutputTexture");

            renderGraph.AddPass(
                std::format("GTAOTemporalFiltering (Compute, {}x{})", temporalFilteringOutputTextureDescription.width, temporalFilteringOutputTextureDescription.height),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                    builder.SetBindlessResourceSRV(1, temporalFilteringInputTexture);
                    builder.SetBindlessResourceSRV(2, temporalFilteringHistoryTexture);
                    builder.SetBindlessResourceSRV(3, intermediateResources.depthTexture);
                    builder.SetBindlessResourceSRV(4, intermediateResources.motionVectorTexture);
                    builder.SetBindlessResourceUAV(5, temporalFilteringOutputTexture, 0);

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::GTAOTemporalFiltering);

                    uint32 threadGroupCountX = Math::CeilDiv(temporalFilteringOutputTextureDescription.width, 8);
                    uint32 threadGroupCountY = Math::CeilDiv(temporalFilteringOutputTextureDescription.height, 8);
                    uint32 threadGroupCountZ = 1;

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    };
                });

            renderGraph.ExportTextureDeferred(temporalFilteringOutputTexture, &historyFrame.ambientOcclusionTexture);

            ambientOcclusionTexture = temporalFilteringOutputTexture;
        }

        if (downsampleFactor > 1)
        {
            RenderGraphTextureHandle upsamplingInputTexture = ambientOcclusionTexture;
            RenderGraphTextureHandle depthTexture = intermediateResources.depthTexture;

            RenderGraphTextureDescription upsamplingOutputTextureDescription = RenderGraphTextureDescription::Create2D(
                renderResolution.width,
                renderResolution.height,
                ambientOcclusionTextureFormat,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
            RenderGraphTextureHandle upsamplingOutputTexture = renderGraph.CreateTexture(upsamplingOutputTextureDescription, "UpsampledAmbientOcclusionTexture");

            renderGraph.AddPass(
                std::format("GTAOUpsampling (Compute, {}x{}->{}x{})", intermediateTextureWidth, intermediateTextureHeight, upsamplingOutputTextureDescription.width, upsamplingOutputTextureDescription.height),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                    builder.SetBindlessResourceSRV(1, upsamplingInputTexture);
                    builder.SetBindlessResourceSRV(2, depthTexture);
                    builder.SetBindlessResourceUAV(3, upsamplingOutputTexture, 0);

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::GTAOUpsampling);

                    uint32 threadGroupCountX = Math::CeilDiv(upsamplingOutputTextureDescription.width, 8);
                    uint32 threadGroupCountY = Math::CeilDiv(upsamplingOutputTextureDescription.height, 8);
                    uint32 threadGroupCountZ = 1;

                    return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                    {
                        RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                        commandList.Dispatch(
                            computeShader,
                            pushConstantValues,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    };
                });

            ambientOcclusionTexture = upsamplingOutputTexture;
        }

        return ambientOcclusionTexture;
    }

    RenderGraphTextureHandle RasterizationRenderer::DispatchAmbientOcclusionDebugVisualization(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

        // TODO
        RenderGraphTextureHandle outputTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");
        //RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(view.targetTexture->GetDesc(), "VisualizeAmbientOcclusionTexture");

        renderGraph.AddPass(
            std::format("VisualizeAmbientOcclusion (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle ambientOcclusionTexture = builder.ReadTexture(intermediateResources.ambientOcclusionTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    uint32 threadGroupCountX = ComputeShaderThreadGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeShaderThreadGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendPushConstantValues pushConstantValues = {};
                    pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(ambientOcclusionTexture));
                    pushConstantValues.BindTextureUAV(2, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndex(outputTexture, 0));

                    RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::VisualizeAmbientOcclusion);

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }
}