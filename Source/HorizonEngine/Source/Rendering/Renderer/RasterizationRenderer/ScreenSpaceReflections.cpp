#include "RasterizationRenderer.h"

namespace Horizon
{
    static constexpr uint32 GScreenSpaceReflectionsThreadGroupSizeX = 8;
    static constexpr uint32 GScreenSpaceReflectionsThreadGroupSizeY = 8;
    static constexpr uint32 GScreenSpaceReflectionsTileSize = 32;

    RenderGraphTextureHandle RasterizationRenderer::RenderScreenSpaceReflections(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
         const uint32 tileCountX = Math::CeilDiv(renderResolution.width, GScreenSpaceReflectionsTileSize);
         const uint32 tileCountY = Math::CeilDiv(renderResolution.height, GScreenSpaceReflectionsTileSize);

         RenderGraphTextureHandle tileClassificationHorizontalTexture = renderGraph.CreateTexture(
             RenderGraphTextureDescription::Create2D(
                 tileCountX,
                 view.targetHeight,
                 RenderBackendTextureFormat::R16G16Float,
                 RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess),
             "SSRTileClassificationHorizontalTexture");

         RenderGraphTextureHandle tileClassificationTexture = renderGraph.CreateTexture(
             RenderGraphTextureDescription::Create2D(
                 tileCountX,
                 tileCountY,
                 RenderBackendTextureFormat::R16G16Float,
                 RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess),
             "SSRTileClassificationTexture");

        RenderGraphBufferDescription rayAllocationBufferDescription = RenderGraphBufferDescription::CreateIndirectArguments(sizeof(uint32), 12);
        RenderGraphBufferHandle rayAllocationBuffer = renderGraph.CreateBuffer(rayAllocationBufferDescription, "SSRRayAllocationBuffer");

        renderGraph.AddPass(
            std::format("SSRClearRayAllocationBufferBuffer ({} bytes)", rayAllocationBufferDescription.size),
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                rayAllocationBuffer = builder.WriteBuffer(rayAllocationBuffer, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    commandList.ClearBufferUAV(resourceRegistry.GetRenderBackendBufferHandle(rayAllocationBuffer), 0);
                };
            });

//
//         RenderGraphTextureHandle earlyExitTilesBuffer = renderGraph.CreateTexture(
//             RenderGraphTextureDesc::Create2D(
//                 numTilesX * numTilesY,
//                 1,
//                 RenderBackendTextureFormat::R32Uint,
//                 RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//             "SSREarlyExitTilesBuffer");
//
//         RenderGraphTextureHandle cheapTilesBuffer = renderGraph.CreateTexture(
//             RenderGraphTextureDesc::Create2D(
//                 numTilesX * numTilesY,
//                 1,
//                 RenderBackendTextureFormat::R32Uint,
//                 RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//             "SSRCheapTilesBuffer");
//
//         RenderGraphTextureHandle expensiveTilesBuffer = renderGraph.CreateTexture(
//             RenderGraphTextureDesc::Create2D(
//                 numTilesX * numTilesY,
//                 1,
//                 RenderBackendTextureFormat::R32Uint,
//                 RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//             "SSRExpensiveTilesBuffer");
//

    RasterizationRendererIntermediateResources& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererIntermediateResources>();

    renderGraph.AddPass(
        "SSRTileClassificationHorizontalPass",
        RenderGraphPassFlags::Compute,
        [&](RenderGraphBuilder& builder)
        {
            builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
            builder.SetBindlessResourceSRV(1, intermediateResources.depthTexture);
            builder.SetBindlessResourceSRV(2, intermediateResources.gbuffer1);
            builder.SetBindlessResourceUAV(3, tileClassificationHorizontalTexture, 0);

            RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SSRTileClassificationHorizontal);

            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(tileCountX, GScreenSpaceReflectionsThreadGroupSizeX);
            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(view.targetHeight, GScreenSpaceReflectionsThreadGroupSizeY);
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
        "SSRTileClassificationVerticalPass",
        RenderGraphPassFlags::Compute,
        [&](RenderGraphBuilder& builder)
        {
            builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
            builder.SetBindlessResourceSRV(1, intermediateResources.depthTexture);
            builder.SetBindlessResourceSRV(2, intermediateResources.gbuffer1);
            builder.SetBindlessResourceSRV(3, tileClassificationHorizontalTexture);
            builder.SetBindlessResourceUAV(4, tileClassificationTexture, 0);
            builder.SetBindlessResourceUAV(5, rayAllocationBuffer);

            RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SSRTileClassificationVertical);

            uint32 threadGroupCountX = ComputeShaderThreadGroupCount(tileCountX, GScreenSpaceReflectionsThreadGroupSizeX);
            uint32 threadGroupCountY = ComputeShaderThreadGroupCount(tileCountY, GScreenSpaceReflectionsThreadGroupSizeY);
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
        "SSRRayAllocationPass",
        RenderGraphPassFlags::Compute,
        [&](RenderGraphBuilder& builder)
        {
            builder.SetBindlessResourceUAV(0, rayAllocationBuffer);

            RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SSRRayAllocation);

            return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
            {
                RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                commandList.Dispatch(
                    computeShader,
                    pushConstantValues,
                    1,
                    1,
                    1);
            };
        });

        RenderGraphTextureHandle rayHitTexture = renderGraph.CreateTexture(
            RenderGraphTextureDescription::Create2D(
                renderResolution.width,
                renderResolution.height,
                RenderBackendTextureFormat::R16G16B16A16Float,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess),
            "SSRRayHitTexture");

        RenderGraphTextureHandle rayColorTexture = renderGraph.CreateTexture(
            RenderGraphTextureDescription::Create2D(
                renderResolution.width,
                renderResolution.height,
                RenderBackendTextureFormat::R16G16B16A16Float,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess),
            "SSRRayColorTexture");
//
//         RenderGraphTextureDesc rayLengthTextureDesc = RenderGraphTextureDesc::Create2D(
//             rayCastingResolutionX,
//             rayCastingResolutionY,
//             RenderBackendTextureFormat::R16Float,
        //     RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess),
//         RenderGraphTextureHandle rayLengthTexture = renderGraph.CreateTexture(rayLengthTextureDesc, "SSRRayLengthTexture");
//
//         const Vector2f hzbUVFactor = Vector2f(
//             float(perFrameData.paramaters.renderResolution.width) / float(hzbWidth),
//             float(perFrameData.paramaters.renderResolution.height) / float(hzbHeight));
//         Vector4f hzbUVFactorAndInvFactor = Vector4f(hzbUVFactor.x, hzbUVFactor.y, 1.0f / hzbUVFactor.x, 1.0 / hzbUVFactor.y);

#if 0
        renderGraph.AddPass(
            "SSRRayTracingPass-EarlyExit (Compute, Indirect)",
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, intermediateResources.gbuffer1);
                builder.SetBindlessResourceSRV(2, intermediateResources.depthTexture);
                builder.SetBindlessResourceSRV(3, intermediateResources.minDepthPyramidTexture);
                builder.SetBindlessResourceSRV(4, intermediateResources.motionVectorTexture);
                builder.SetBindlessResourceSRV(6, previousColorTexture);
                builder.SetBindlessResourceUAV(7, rayIndirectSpecular, 0);
                builder.SetBindlessResourceUAV(8, rayDirectionAndLengthTexture, 0);
                builder.SetShaderConstantValue(9, static_cast<float>(rayCastingResolutionX));
                builder.SetShaderConstantValue(10, static_cast<float>(rayCastingResolutionY));
                builder.SetShaderConstantValue(11, hzbUVFactorAndInvFactor.x);
                builder.SetShaderConstantValue(12, hzbUVFactorAndInvFactor.y);
                builder.SetShaderConstantValue(13, hzbUVFactorAndInvFactor.z);
                builder.SetShaderConstantValue(14, hzbUVFactorAndInvFactor.w);

                RenderBackendShaderHandle computeShader = shaderCollection->GetShader(ShaderID::SSRRayTracingEarlyExit);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.DispatchIndirect(
                        computeShader,
                        pushConstantValues,
                        resourceRegistry.GetRenderBackendBufferHandle(rayAllocationBuffer),
                        24); // @todo offset
                };
            });
#endif

        RenderGraphTextureHandle inputColorTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        if (historyFrame.temporalSuperSamplingOutputTexture)
        {
            inputColorTexture = renderGraph.ImportExternalTexture(historyFrame.temporalSuperSamplingOutputTexture, "SSRInputColorTexture");
        }

        renderGraph.AddPass(
            "SSRRayTracingPass-Cheap (Compute, Indirect)",
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, inputColorTexture);
                builder.SetBindlessResourceSRV(2, intermediateResources.depthTexture);
                builder.SetBindlessResourceSRV(3, intermediateResources.minDepthPyramidTexture);
                builder.SetBindlessResourceSRV(4, intermediateResources.gbuffer0);
                builder.SetBindlessResourceSRV(5, intermediateResources.gbuffer1);
                builder.SetBindlessResourceUAV(6, rayHitTexture, 0);
                builder.SetBindlessResourceUAV(7, rayColorTexture, 0);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SSRRayTracingCheap);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.Dispatch(
                        computeShader,
                        pushConstantValues,
                        ComputeShaderThreadGroupCount(renderResolution.width, GScreenSpaceReflectionsThreadGroupSizeX),
                        ComputeShaderThreadGroupCount(renderResolution.height, GScreenSpaceReflectionsThreadGroupSizeY),
                        1); // @todo offset
                    // commandList.DispatchIndirect(
                    //     computeShader,
                    //     pushConstantValues,
                    //     resourceRegistry.GetRenderBackendBufferHandle(rayAllocationBuffer),
                    //     24); // @todo offset
                };
            });

        RenderGraphTextureHandle colorResolveTexture = renderGraph.CreateTexture(
            RenderGraphTextureDescription::Create2D(
                renderResolution.width,
                renderResolution.height,
                RenderBackendTextureFormat::R16G16B16A16Float,
                RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess),
            "SSRColorResolveTexture");
//
//         RenderGraphTextureHandle resolveVariance = renderGraph.CreateTexture(
//             RenderGraphTextureDesc::Create2D(
//                 view.targetWidth,
//                 view.targetHeight,
//                 RenderBackendTextureFormat::R16Float,
//                 RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//             "SSRResolveVariance");
//
//         RenderGraphTextureHandle reprojectionDepth = renderGraph.CreateTexture(
//             RenderGraphTextureDesc::Create2D(
//                 view.targetWidth,
//                 view.targetHeight,
//                 RenderBackendTextureFormat::R16Float,
//                 RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//             "SSRReprojectionDepth");

        renderGraph.AddPass(
            "SSRColorResolvePass",
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, GetCurrentPerFrameConstantBuffer());
                builder.SetBindlessResourceSRV(1, intermediateResources.depthTexture);
                builder.SetBindlessResourceSRV(2, intermediateResources.gbuffer0);
                builder.SetBindlessResourceSRV(3, intermediateResources.gbuffer1);
                builder.SetBindlessResourceSRV(4, rayHitTexture);
                builder.SetBindlessResourceSRV(5, rayColorTexture);
                builder.SetBindlessResourceUAV(6, colorResolveTexture, 0);

                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SSRColorResolve);

                uint32 threadGroupCountX = ComputeShaderThreadGroupCount(renderResolution.width, GScreenSpaceReflectionsThreadGroupSizeX);
                uint32 threadGroupCountY = ComputeShaderThreadGroupCount(renderResolution.height, GScreenSpaceReflectionsThreadGroupSizeY);
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

        RenderGraphTextureHandle screenSpaceReflectionTexture = colorResolveTexture;
//
//         if (settings.denosingEnabled)
//         {
//             RenderGraphTextureDesc temporalFilteringOutputTextureDesc = RenderGraphTextureDesc::Create2D(
//                 view.targetWidth,
//                 view.targetHeight,
//                 RenderBackendTextureFormat::RGBA16Float,
//                 RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
//             RenderGraphTextureHandle temporalFilteringOutputTexture = renderGraph.CreateTexture(
//                 temporalFilteringOutputTextureDesc,
//                 "SSRTemporalFilteringOutputTexture");
//
//             RenderGraphTextureDesc temporalVarianceTextureDesc = RenderGraphTextureDesc::Create2D(
//                 view.targetWidth,
//                 view.targetHeight,
//                 RenderBackendTextureFormat::R16Float,
//                 RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
//             RenderGraphTextureHandle temporalVarianceTexture = renderGraph.CreateTexture(
//                 temporalVarianceTextureDesc,
//                 "SSRTemporalVarianceTexture");
//
//             if (!ssrTemporalFilteringOutputTextureCache.texture || (ssrTemporalFilteringOutputTextureCache.desc != temporalFilteringOutputTextureDesc))
//             {
//                 ssrTemporalFilteringOutputTextureCache.desc = temporalFilteringOutputTextureDesc;
//                 ssrTemporalFilteringOutputTextureCache.desc.initialState = RenderBackendResourceState::ShaderResource;
//                 ssrTemporalFilteringOutputTextureCache.texture = renderBackend->CreateTexture(deviceMask, &ssrTemporalFilteringOutputTextureCache.desc, nullptr, "SSRHistoryTemporalFilteringOutputTexture");
//                 ssrTemporalFilteringOutputTextureCache.initialState = RenderBackendResourceState::ShaderResource;
//             }
//             RenderGraphTextureHandle historyTemporalFilteringOutputTexture = renderGraph.ImportExternalTexture(ssrTemporalFilteringOutputTextureCache.texture, ssrTemporalFilteringOutputTextureCache.desc, ssrTemporalFilteringOutputTextureCache.initialState, "SSRHistoryTemporalFilteringOutputTexture");
//
//             if (!ssrTemporalVarianceTextureCache.texture || (ssrTemporalVarianceTextureCache.desc != temporalVarianceTextureDesc))
//             {
//                 ssrTemporalVarianceTextureCache.desc = temporalVarianceTextureDesc;
//                 ssrTemporalVarianceTextureCache.desc.initialState = RenderBackendResourceState::ShaderResource;
//                 ssrTemporalVarianceTextureCache.texture = renderBackend->CreateTexture(deviceMask, &ssrTemporalVarianceTextureCache.desc, nullptr, "SSRHistoryTemporalVarianceTexture");
//                 ssrTemporalVarianceTextureCache.initialState = RenderBackendResourceState::ShaderResource;
//             }
//             RenderGraphTextureHandle historyTemporalVarianceTexture = renderGraph.ImportExternalTexture(ssrTemporalVarianceTextureCache.texture, ssrTemporalVarianceTextureCache.desc, ssrTemporalVarianceTextureCache.initialState, "SSRHistoryTemporalVarianceTexture");
//
//             renderGraph.AddPass("SSRTemporalFilteringPass", RenderGraphPassFlags::Compute,
//                 [&](RenderGraphBuilder& builder)
//                 {
//                     const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//                     const auto& sceneDepthData = blackboard.Get<RasterizationRendererHistoryInfo>();
//                     auto& intermediateResources = blackboard.Get<RasterizationRendererSceneTextures>();
//
//                     auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                     auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                     auto motionVectors = builder.ReadTexture(intermediateResources.motionVectorTexture, RenderBackendResourceState::ShaderResource);
//                     resolveTexture = builder.ReadTexture(resolveTexture, RenderBackendResourceState::ShaderResource);
//                     resolveVariance = builder.ReadTexture(resolveVariance, RenderBackendResourceState::ShaderResource);
//                     reprojectionDepth = builder.ReadTexture(reprojectionDepth, RenderBackendResourceState::ShaderResource);
//                     historyTemporalFilteringOutputTexture = builder.ReadTexture(historyTemporalFilteringOutputTexture, RenderBackendResourceState::ShaderResource);
//                     historyTemporalVarianceTexture = builder.ReadTexture(historyTemporalVarianceTexture, RenderBackendResourceState::ShaderResource);
//                     historySceneDepth = builder.ReadTexture(historySceneDepth, RenderBackendResourceState::ShaderResource);
//
//                     temporalFilteringOutputTexture = builder.WriteTexture(temporalFilteringOutputTexture, RenderBackendResourceState::UnorderedAccess);
//                     temporalVarianceTexture = builder.WriteTexture(temporalVarianceTexture, RenderBackendResourceState::UnorderedAccess);
//
//
//                     {
//                         uint32 threadGroupCountX = ComputeWorkGroupCount(view.targetWidth, SSR_THREAD_GROUP_SIZE);
//                         uint32 threadGroupCountY = ComputeWorkGroupCount(view.targetHeight, SSR_THREAD_GROUP_SIZE);
//
//                         RenderBackendPushConstantValues pushConstantValues = {};
//                         pushConstantValues.BindBuffer(0, perFrameData.buffer, 0);
//                         pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepth)));
//                         pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1)));
//                         pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(motionVectors)));
//                         pushConstantValues.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(resolveTexture)));
//                         pushConstantValues.BindTextureSRV(5, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(resolveVariance)));
//                         pushConstantValues.BindTextureSRV(6, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(reprojectionDepth)));
//                         pushConstantValues.BindTextureSRV(7, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(historyTemporalFilteringOutputTexture)));
//                         pushConstantValues.BindTextureSRV(8, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(historyTemporalVarianceTexture)));
//                         pushConstantValues.BindTextureSRV(9, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(historySceneDepth)));
//                         pushConstantValues.BindTextureUAV(10, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndextemporalFilteringOutputTexture), 0));
//                         pushConstantValues.BindTextureUAV(11, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndextemporalVarianceTexture), 0));
//
//                         auto temporalFilteringCS = shaderRepository->GetShader(ShaderID::SSRTemporalFiltering);
//                         commandList.Dispatch2D(
//                             temporalFilteringCS,
//                             pushConstantValues,
//                             threadGroupCountX,
//                             groupCountY);
//                     };
//                 });
//
//             renderGraph.ExportTextureDeferred(temporalFilteringOutputTexture, &ssrTemporalFilteringOutputTextureCache);
//             renderGraph.ExportTextureDeferred(temporalVarianceTexture, &ssrTemporalVarianceTextureCache);
//
//             RenderGraphTextureHandle spatialFilteringIntermediateTexture = renderGraph.CreateTexture(
//                 RenderGraphTextureDesc::Create2D(
//                     view.targetWidth,
//                     view.targetHeight,
//                     RenderBackendTextureFormat::RGBA16Float,
//                     RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//                 "SSRSpatialFilteringIntermediateTexture");
//
//             renderGraph.AddPass("SSRSpatialFilteringHorizontalPass", RenderGraphPassFlags::Compute,
//                 [&](RenderGraphBuilder& builder)
//                 {
//                     const auto& sceneDepthData = blackboard.Get<RasterizationRendererHistoryInfo>();
//                     const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//
//                     auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                     auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                     resolveVariance = builder.ReadTexture(resolveVariance, RenderBackendResourceState::ShaderResource);
//                     temporalFilteringOutputTexture = builder.ReadTexture(temporalFilteringOutputTexture, RenderBackendResourceState::ShaderResource);
//
//                     spatialFilteringIntermediateTexture = builder.WriteTexture(spatialFilteringIntermediateTexture, RenderBackendResourceState::UnorderedAccess);
//
//
//                     {
//                         uint32 threadGroupCountX = ComputeWorkGroupCount(view.targetWidth, SSR_THREAD_GROUP_SIZE);
//                         uint32 threadGroupCountY = ComputeWorkGroupCount(view.targetHeight, SSR_THREAD_GROUP_SIZE);
//
//                         RenderBackendPushConstantValues pushConstantValues = {};
//                         pushConstantValues.BindBuffer(0, perFrameData.buffer, 0);
//                         pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepth)));
//                         pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1)));
//                         pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(resolveVariance)));
//                         pushConstantValues.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(temporalFilteringOutputTexture)));
//                         pushConstantValues.BindTextureUAV(5, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndexspatialFilteringIntermediateTexture), 0));
//                         pushConstantValues.PushConstants(0, 1.0f);
//                         pushConstantValues.PushConstants(1, 0.0f);
//
//                         auto spatialFilteringCS = shaderRepository->GetShader(ShaderID::SSRSpatialFiltering);
//                         commandList.Dispatch2D(
//                             spatialFilteringCS,
//                             pushConstantValues,
//                             threadGroupCountX,
//                             groupCountY);
//                     };
//                 });
//
//             renderGraph.AddPass("SSRSpatialFilteringVerticalPass", RenderGraphPassFlags::Compute,
//                 [&](RenderGraphBuilder& builder)
//                 {
//                     const auto& sceneDepthData = blackboard.Get<RasterizationRendererHistoryInfo>();
//                     const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//
//                     auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                     auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                     resolveVariance = builder.ReadTexture(resolveVariance, RenderBackendResourceState::ShaderResource);
//                     spatialFilteringIntermediateTexture = builder.ReadTexture(spatialFilteringIntermediateTexture, RenderBackendResourceState::ShaderResource);
//
//                     ssrTexture = builder.WriteTexture(ssrTexture, RenderBackendResourceState::UnorderedAccess);
//
//
//                     {
//                         uint32 threadGroupCountX = ComputeWorkGroupCount(view.targetWidth, SSR_THREAD_GROUP_SIZE);
//                         uint32 threadGroupCountY = ComputeWorkGroupCount(view.targetHeight, SSR_THREAD_GROUP_SIZE);
//
//                         RenderBackendPushConstantValues pushConstantValues = {};
//                         pushConstantValues.BindBuffer(0, perFrameData.buffer, 0);
//                         pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepth)));
//                         pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1)));
//                         pushConstantValues.BindTextureSRV(3, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(resolveVariance)));
//                         pushConstantValues.BindTextureSRV(4, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(spatialFilteringIntermediateTexture)));
//                         pushConstantValues.BindTextureUAV(5, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndexssrTexture), 0));
//                         pushConstantValues.PushConstants(0, 0.0f);
//                         pushConstantValues.PushConstants(1, 1.0f);
//
//                         auto spatialFilteringCS = shaderRepository->GetShader(ShaderID::SSRSpatialFiltering);
//                         commandList.Dispatch2D(
//                             spatialFilteringCS,
//                             pushConstantValues,
//                             threadGroupCountX,
//                             groupCountY);
//                     };
//                 });
//         }

        return screenSpaceReflectionTexture;
    }
}