//#include "RealTimeRenderer.h"
//
//#define SSR_THREAD_GROUP_SIZE 8
//
//namespace HE
//{
//    void RealTimeRenderer::RenderScreenSpaceReflections(
//        RenderGraph& renderGraph,
//        RenderGraphBlackboard& blackboard,
//        const SceneView& view,
//        uint32 hzbWidth,
//        uint32 hzbHeight,
//        RenderGraphTextureHandle hzb,
//        RenderGraphTextureHandle historySceneColor,
//        RenderGraphTextureHandle historySceneDepth,
//        RenderBackendBufferHandle rayAllocationBuffer,
//        RenderGraphTextureHandle& ssrTexture,
//        RenderGraphTextureHandle& debugOutputTexture)
//    {
//        uint32 deviceMask = ~0u;
//        const auto& perFrameData = blackboard.Get<RealTimeRendererSceneViewInfo>();
//
//        const uint32 tileSize = 32;
//        const uint32 numTilesX = Math::CeilDiv(perFrameData.paramaters.renderResolutionX, tileSize);
//        const uint32 numTilesY = Math::CeilDiv(perFrameData.paramaters.renderResolutionY, tileSize);
//
//        const auto& settings = this->settings.ssrSettings;
//
//        RenderGraphTextureHandle tileClassificationHorizontalBuffer = renderGraph.CreateTexture(
//            RenderGraphTextureDesc::Create2D(
//                numTilesX,
//                view.targetHeight,
//                RenderBackendTextureFormat::RG16Float,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//            "SSRTileClassificationHorizontalBuffer");
//
//        RenderGraphTextureHandle tileClassificationBuffer = renderGraph.CreateTexture(
//            RenderGraphTextureDesc::Create2D(
//                numTilesX,
//                numTilesY,
//                RenderBackendTextureFormat::RG16Float,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//            "SSRTileClassificationBuffer");
//
//        RenderGraphTextureHandle earlyExitTilesBuffer = renderGraph.CreateTexture(
//            RenderGraphTextureDesc::Create2D(
//                numTilesX * numTilesY,
//                1,
//                RenderBackendTextureFormat::R32Uint,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//            "SSREarlyExitTilesBuffer");
//
//        RenderGraphTextureHandle cheapTilesBuffer = renderGraph.CreateTexture(
//            RenderGraphTextureDesc::Create2D(
//                numTilesX * numTilesY,
//                1,
//                RenderBackendTextureFormat::R32Uint,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//            "SSRCheapTilesBuffer");
//
//        RenderGraphTextureHandle expensiveTilesBuffer = renderGraph.CreateTexture(
//            RenderGraphTextureDesc::Create2D(
//                numTilesX * numTilesY,
//                1,
//                RenderBackendTextureFormat::R32Uint,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//            "SSRExpensiveTilesBuffer");
//
//        renderGraph.AddPass("SSRTileClassificationHorizontalPass", RenderGraphPassFlags::Compute,
//            [&](RenderGraphBuilder& builder)
//            {
//                const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//                const auto& sceneDepthData = blackboard.Get<RealTimeRendererHistoryInfo>();
//
//                auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//
//                tileClassificationHorizontalBuffer = builder.WriteTexture(tileClassificationHorizontalBuffer, RenderBackendResourceState::UnorderedAccess);
//
//                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                {
//                    uint32 dispatchX = Math::CeilDiv(numTilesX, SSR_THREAD_GROUP_SIZE);
//                    uint32 dispatchY = Math::CeilDiv(view.targetHeight, SSR_THREAD_GROUP_SIZE);
//
//                    RenderBackendShaderArguments shaderArguments = {};
//                    shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneDepth)));
//                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer1)));
//                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(tileClassificationHorizontalBuffer), 0));
//
//                    auto tileClassificationCS = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SSRTileClassificationHorizontal);
//                    commandList.Dispatch2D(
//                        tileClassificationCS,
//                        shaderArguments,
//                        dispatchX,
//                        dispatchY);
//                };
//            });
//
//        renderGraph.AddPass("SSRTileClassificationVerticalPass", RenderGraphPassFlags::Compute,
//            [&](RenderGraphBuilder& builder)
//            {
//                const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//                const auto& sceneDepthData = blackboard.Get<RealTimeRendererHistoryInfo>();
//
//                auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                tileClassificationHorizontalBuffer = builder.ReadTexture(tileClassificationHorizontalBuffer, RenderBackendResourceState::ShaderResource);
//
//                tileClassificationBuffer = builder.WriteTexture(tileClassificationBuffer, RenderBackendResourceState::UnorderedAccess);
//                earlyExitTilesBuffer = builder.WriteTexture(earlyExitTilesBuffer, RenderBackendResourceState::UnorderedAccess);
//                cheapTilesBuffer = builder.WriteTexture(cheapTilesBuffer, RenderBackendResourceState::UnorderedAccess);
//                expensiveTilesBuffer = builder.WriteTexture(expensiveTilesBuffer, RenderBackendResourceState::UnorderedAccess);
//
//                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                {
//                    uint32 dispatchX = Math::CeilDiv(numTilesX, SSR_THREAD_GROUP_SIZE);
//                    uint32 dispatchY = Math::CeilDiv(numTilesY, SSR_THREAD_GROUP_SIZE);
//
//                    RenderBackendShaderArguments shaderArguments = {};
//                    shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneDepth)));
//                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer1)));
//                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(tileClassificationHorizontalBuffer)));
//                    shaderArguments.BindTextureUAV(4, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(tileClassificationBuffer), 0));
//                    shaderArguments.BindTextureUAV(5, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(earlyExitTilesBuffer), 0));
//                    shaderArguments.BindTextureUAV(6, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(cheapTilesBuffer), 0));
//                    shaderArguments.BindTextureUAV(7, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(expensiveTilesBuffer), 0));
//                    shaderArguments.BindBuffer(8, rayAllocationBuffer, 0);
//
//                    auto tileClassificationCS = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SSRTileClassificationVertical);
//                    commandList.Dispatch2D(
//                        tileClassificationCS,
//                        shaderArguments,
//                        dispatchX,
//                        dispatchY);
//                };
//            });
//
//        renderGraph.AddPass("SSRRayAllocationPass", RenderGraphPassFlags::Compute,
//            [&](RenderGraphBuilder& builder)
//            {
//                // rayAllocationBuffer = builder.WriteBuffer(rayAllocationBuffer, RenderBackendResourceState::UnorderedAccess);
//
//                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                {
//                    RenderBackendBarrier transitions[] =
//                    {
//                        RenderBackendBarrier(rayAllocationBuffer, RenderBackendBufferSubresourceRange(0, 48), RenderBackendResourceState::UnorderedAccess, RenderBackendResourceState::IndirectArgument)
//                    };
//                    commandList.Transitions(transitions, 1);
//
//                    RenderBackendShaderArguments shaderArguments = {};
//                    shaderArguments.BindBuffer(0, rayAllocationBuffer, 0);
//
//                    auto rayAllocationCS = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SSRRayAllocation);
//                    commandList.Dispatch(
//                        rayAllocationCS,
//                        shaderArguments,
//                        1, 1, 1);
//                };
//            });
//
//        // Half-resolution
//#if 0
//        uint32 rayCastingResolutionX = view.targetWidth / 2;
//        uint32 rayCastingResolutionY = view.targetHeight / 2;
//#else
//        uint32 rayCastingResolutionX = view.targetWidth;
//        uint32 rayCastingResolutionY = view.targetHeight;
//#endif
//
//        RenderGraphTextureHandle rayIndirectSpecular = renderGraph.CreateTexture(
//            RenderGraphTextureDesc::Create2D(
//                rayCastingResolutionX,
//                rayCastingResolutionY,
//                RenderBackendTextureFormat::RGBA16Float,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//            "SSRRayIndirectSpecular");
//
//        RenderGraphTextureHandle rayDirectionPDF = renderGraph.CreateTexture(
//            RenderGraphTextureDesc::Create2D(
//                rayCastingResolutionX,
//                rayCastingResolutionY,
//                RenderBackendTextureFormat::RGBA16Float,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//            "SSRRayDirectionPDF");
//
//        RenderGraphTextureHandle rayLength = renderGraph.CreateTexture(
//            RenderGraphTextureDesc::Create2D(
//                rayCastingResolutionX,
//                rayCastingResolutionY,
//                RenderBackendTextureFormat::R16Float,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//            "SSRRayLength");
//
//        const Vector2 hzbUVFactor = Vector2(
//            float(perFrameData.paramaters.renderResolutionX) / float(hzbWidth),
//            float(perFrameData.paramaters.renderResolutionY) / float(hzbHeight));
//        Vector4 hzbUVFactorAndInvFactor = Vector4(hzbUVFactor.x, hzbUVFactor.y, 1.0f / hzbUVFactor.x, 1.0 / hzbUVFactor.y);
//
//        renderGraph.AddPass("SSRRayCastingPass", RenderGraphPassFlags::Compute,
//            [&](RenderGraphBuilder& builder)
//            {
//                const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//                const auto& sceneDepthData = blackboard.Get<RealTimeRendererHistoryInfo>();
//                auto& sceneTextures = blackboard.Get<RealTimeRendererSceneTextures>();
//
//                auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                auto motionVectors = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);
//                auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                hzb = builder.ReadTexture(hzb, RenderBackendResourceState::ShaderResource);
//                earlyExitTilesBuffer = builder.ReadTexture(earlyExitTilesBuffer, RenderBackendResourceState::ShaderResource);
//                cheapTilesBuffer = builder.ReadTexture(cheapTilesBuffer, RenderBackendResourceState::ShaderResource);
//                expensiveTilesBuffer = builder.ReadTexture(expensiveTilesBuffer, RenderBackendResourceState::ShaderResource);
//
//                rayIndirectSpecular = builder.WriteTexture(rayIndirectSpecular, RenderBackendResourceState::UnorderedAccess);
//                rayDirectionPDF = builder.WriteTexture(rayDirectionPDF, RenderBackendResourceState::UnorderedAccess);
//                rayLength = builder.WriteTexture(rayLength, RenderBackendResourceState::UnorderedAccess);
//
//                debugOutputTexture = builder.WriteTexture(debugOutputTexture, RenderBackendResourceState::UnorderedAccess);
//
//                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                {
//                    RenderBackendShaderArguments shaderArguments = {};
//                    shaderArguments.BindBuffer(0, perFrameData.buffer, 0);
//                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer1)));
//                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(motionVectors)));
//                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneDepth)));
//                    shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(hzb)));
//                    shaderArguments.BindTextureSRV(6, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(historySceneColor)));
//                    shaderArguments.BindTextureUAV(7, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(rayIndirectSpecular), 0));
//                    shaderArguments.BindTextureUAV(8, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(rayDirectionPDF), 0));
//                    shaderArguments.BindTextureUAV(9, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(rayLength), 0));
//                    shaderArguments.BindTextureSRV(10, RenderBackendTextureSRVDesc::Create(blueNoiseTexture));
//
//                    shaderArguments.BindTextureUAV(15, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(debugOutputTexture), 0));
//
//                    shaderArguments.PushConstants(0, float(rayCastingResolutionX));
//                    shaderArguments.PushConstants(1, float(rayCastingResolutionY));
//                    shaderArguments.PushConstants(2, hzbUVFactorAndInvFactor.x);
//                    shaderArguments.PushConstants(3, hzbUVFactorAndInvFactor.y);
//                    shaderArguments.PushConstants(4, hzbUVFactorAndInvFactor.z);
//                    shaderArguments.PushConstants(5, hzbUVFactorAndInvFactor.w);
//
//                    shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(earlyExitTilesBuffer)));
//
//                    commandList.ClearTexture(RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(debugOutputTexture), 0), RenderBackendTextureClearValue::CreateColorValueFloat4(0.0f, 0.0f, 0.0f, 0.0f));
//
//                    auto earlyExitRaysCS = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SSRDispatchEarlyExitRays);
//                    commandList.DispatchIndirect(
//                        earlyExitRaysCS,
//                        shaderArguments,
//                        rayAllocationBuffer,
//                        12);
//
//                    shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(cheapTilesBuffer)));
//
//                    auto cheapRaysCS = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SSRDispatchCheapRays);
//                    commandList.DispatchIndirect(
//                        cheapRaysCS,
//                        shaderArguments,
//                        rayAllocationBuffer,
//                        24);
//
//                    shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(expensiveTilesBuffer)));
//
//                    auto expensiveRaysCS = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SSRDispatchExpensiveRays);
//                    commandList.DispatchIndirect(
//                        expensiveRaysCS,
//                        shaderArguments,
//                        rayAllocationBuffer,
//                        36);
//                };
//            });
//
//        RenderGraphTextureHandle resolveTexture = renderGraph.CreateTexture(
//            RenderGraphTextureDesc::Create2D(
//                view.targetWidth,
//                view.targetHeight,
//                RenderBackendTextureFormat::RGBA16Float,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//            "SSRResolveTexture");
//
//        RenderGraphTextureHandle resolveVariance = renderGraph.CreateTexture(
//            RenderGraphTextureDesc::Create2D(
//                view.targetWidth,
//                view.targetHeight,
//                RenderBackendTextureFormat::R16Float,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//            "SSRResolveVariance");
//
//        RenderGraphTextureHandle reprojectionDepth = renderGraph.CreateTexture(
//            RenderGraphTextureDesc::Create2D(
//                view.targetWidth,
//                view.targetHeight,
//                RenderBackendTextureFormat::R16Float,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//            "SSRReprojectionDepth");
//
//        renderGraph.AddPass("SSRResolvePass", RenderGraphPassFlags::Compute,
//            [&](RenderGraphBuilder& builder)
//            {
//                const auto& sceneDepthData = blackboard.Get<RealTimeRendererHistoryInfo>();
//                const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//
//                auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                rayIndirectSpecular = builder.ReadTexture(rayIndirectSpecular, RenderBackendResourceState::ShaderResource);
//                rayDirectionPDF = builder.ReadTexture(rayDirectionPDF, RenderBackendResourceState::ShaderResource);
//                rayLength = builder.ReadTexture(rayLength, RenderBackendResourceState::ShaderResource);
//
//                resolveTexture = builder.WriteTexture(resolveTexture, RenderBackendResourceState::UnorderedAccess);
//                resolveVariance = builder.WriteTexture(resolveVariance, RenderBackendResourceState::UnorderedAccess);
//                reprojectionDepth = builder.WriteTexture(reprojectionDepth, RenderBackendResourceState::UnorderedAccess);
//
//                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                {
//                    uint32 dispatchX = Math::CeilDiv(view.targetWidth, SSR_THREAD_GROUP_SIZE);
//                    uint32 dispatchY = Math::CeilDiv(view.targetHeight, SSR_THREAD_GROUP_SIZE);
//
//                    RenderBackendShaderArguments shaderArguments = {};
//                    shaderArguments.BindBuffer(0, perFrameData.buffer, 0);
//                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneDepth)));
//                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer1)));
//                    shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(rayIndirectSpecular)));
//                    shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(rayDirectionPDF)));
//                    shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(rayLength)));
//                    shaderArguments.BindTextureUAV(6, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(resolveTexture), 0));
//                    shaderArguments.BindTextureUAV(7, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(resolveVariance), 0));
//                    shaderArguments.BindTextureUAV(8, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(reprojectionDepth), 0));
//
//                    auto resolveCS = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SSRResolve);
//                    commandList.Dispatch2D(
//                        resolveCS,
//                        shaderArguments,
//                        dispatchX,
//                        dispatchY);
//                };
//            });
//
//        if (settings.denosingEnabled)
//        {
//            RenderGraphTextureDesc temporalFilteringOutputTextureDesc = RenderGraphTextureDesc::Create2D(
//                view.targetWidth,
//                view.targetHeight,
//                RenderBackendTextureFormat::RGBA16Float,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
//            RenderGraphTextureHandle temporalFilteringOutputTexture = renderGraph.CreateTexture(
//                temporalFilteringOutputTextureDesc,
//                "SSRTemporalFilteringOutputTexture");
//
//            RenderGraphTextureDesc temporalVarianceTextureDesc = RenderGraphTextureDesc::Create2D(
//                view.targetWidth,
//                view.targetHeight,
//                RenderBackendTextureFormat::R16Float,
//                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
//            RenderGraphTextureHandle temporalVarianceTexture = renderGraph.CreateTexture(
//                temporalVarianceTextureDesc,
//                "SSRTemporalVarianceTexture");
//
//            if (!ssrTemporalFilteringOutputTextureCache.texture || (ssrTemporalFilteringOutputTextureCache.desc != temporalFilteringOutputTextureDesc))
//            {
//                ssrTemporalFilteringOutputTextureCache.desc = temporalFilteringOutputTextureDesc;
//                ssrTemporalFilteringOutputTextureCache.desc.initialState = RenderBackendResourceState::ShaderResource;
//                ssrTemporalFilteringOutputTextureCache.texture = renderBackend->CreateTexture(deviceMask, &ssrTemporalFilteringOutputTextureCache.desc, nullptr, "SSRHistoryTemporalFilteringOutputTexture");
//                ssrTemporalFilteringOutputTextureCache.initialState = RenderBackendResourceState::ShaderResource;
//            }
//            RenderGraphTextureHandle historyTemporalFilteringOutputTexture = renderGraph.ImportExternalTexture(ssrTemporalFilteringOutputTextureCache.texture, ssrTemporalFilteringOutputTextureCache.desc, ssrTemporalFilteringOutputTextureCache.initialState, "SSRHistoryTemporalFilteringOutputTexture");
//
//            if (!ssrTemporalVarianceTextureCache.texture || (ssrTemporalVarianceTextureCache.desc != temporalVarianceTextureDesc))
//            {
//                ssrTemporalVarianceTextureCache.desc = temporalVarianceTextureDesc;
//                ssrTemporalVarianceTextureCache.desc.initialState = RenderBackendResourceState::ShaderResource;
//                ssrTemporalVarianceTextureCache.texture = renderBackend->CreateTexture(deviceMask, &ssrTemporalVarianceTextureCache.desc, nullptr, "SSRHistoryTemporalVarianceTexture");
//                ssrTemporalVarianceTextureCache.initialState = RenderBackendResourceState::ShaderResource;
//            }
//            RenderGraphTextureHandle historyTemporalVarianceTexture = renderGraph.ImportExternalTexture(ssrTemporalVarianceTextureCache.texture, ssrTemporalVarianceTextureCache.desc, ssrTemporalVarianceTextureCache.initialState, "SSRHistoryTemporalVarianceTexture");
//
//            renderGraph.AddPass("SSRTemporalFilteringPass", RenderGraphPassFlags::Compute,
//                [&](RenderGraphBuilder& builder)
//                {
//                    const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//                    const auto& sceneDepthData = blackboard.Get<RealTimeRendererHistoryInfo>();
//                    auto& sceneTextures = blackboard.Get<RealTimeRendererSceneTextures>();
//
//                    auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                    auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                    auto motionVectors = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);
//                    resolveTexture = builder.ReadTexture(resolveTexture, RenderBackendResourceState::ShaderResource);
//                    resolveVariance = builder.ReadTexture(resolveVariance, RenderBackendResourceState::ShaderResource);
//                    reprojectionDepth = builder.ReadTexture(reprojectionDepth, RenderBackendResourceState::ShaderResource);
//                    historyTemporalFilteringOutputTexture = builder.ReadTexture(historyTemporalFilteringOutputTexture, RenderBackendResourceState::ShaderResource);
//                    historyTemporalVarianceTexture = builder.ReadTexture(historyTemporalVarianceTexture, RenderBackendResourceState::ShaderResource);
//                    historySceneDepth = builder.ReadTexture(historySceneDepth, RenderBackendResourceState::ShaderResource);
//
//                    temporalFilteringOutputTexture = builder.WriteTexture(temporalFilteringOutputTexture, RenderBackendResourceState::UnorderedAccess);
//                    temporalVarianceTexture = builder.WriteTexture(temporalVarianceTexture, RenderBackendResourceState::UnorderedAccess);
//
//                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                    {
//                        uint32 dispatchX = Math::CeilDiv(view.targetWidth, SSR_THREAD_GROUP_SIZE);
//                        uint32 dispatchY = Math::CeilDiv(view.targetHeight, SSR_THREAD_GROUP_SIZE);
//
//                        RenderBackendShaderArguments shaderArguments = {};
//                        shaderArguments.BindBuffer(0, perFrameData.buffer, 0);
//                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneDepth)));
//                        shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer1)));
//                        shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(motionVectors)));
//                        shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(resolveTexture)));
//                        shaderArguments.BindTextureSRV(5, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(resolveVariance)));
//                        shaderArguments.BindTextureSRV(6, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(reprojectionDepth)));
//                        shaderArguments.BindTextureSRV(7, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(historyTemporalFilteringOutputTexture)));
//                        shaderArguments.BindTextureSRV(8, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(historyTemporalVarianceTexture)));
//                        shaderArguments.BindTextureSRV(9, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(historySceneDepth)));
//                        shaderArguments.BindTextureUAV(10, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(temporalFilteringOutputTexture), 0));
//                        shaderArguments.BindTextureUAV(11, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(temporalVarianceTexture), 0));
//
//                        auto temporalFilteringCS = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SSRTemporalFiltering);
//                        commandList.Dispatch2D(
//                            temporalFilteringCS,
//                            shaderArguments,
//                            dispatchX,
//                            dispatchY);
//                    };
//                });
//
//            renderGraph.ExportTextureDeferred(temporalFilteringOutputTexture, &ssrTemporalFilteringOutputTextureCache);
//            renderGraph.ExportTextureDeferred(temporalVarianceTexture, &ssrTemporalVarianceTextureCache);
//
//            RenderGraphTextureHandle spatialFilteringIntermediateTexture = renderGraph.CreateTexture(
//                RenderGraphTextureDesc::Create2D(
//                    view.targetWidth,
//                    view.targetHeight,
//                    RenderBackendTextureFormat::RGBA16Float,
//                    RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//                "SSRSpatialFilteringIntermediateTexture");
//
//            renderGraph.AddPass("SSRSpatialFilteringHorizontalPass", RenderGraphPassFlags::Compute,
//                [&](RenderGraphBuilder& builder)
//                {
//                    const auto& sceneDepthData = blackboard.Get<RealTimeRendererHistoryInfo>();
//                    const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//
//                    auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                    auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                    resolveVariance = builder.ReadTexture(resolveVariance, RenderBackendResourceState::ShaderResource);
//                    temporalFilteringOutputTexture = builder.ReadTexture(temporalFilteringOutputTexture, RenderBackendResourceState::ShaderResource);
//
//                    spatialFilteringIntermediateTexture = builder.WriteTexture(spatialFilteringIntermediateTexture, RenderBackendResourceState::UnorderedAccess);
//
//                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                    {
//                        uint32 dispatchX = Math::CeilDiv(view.targetWidth, SSR_THREAD_GROUP_SIZE);
//                        uint32 dispatchY = Math::CeilDiv(view.targetHeight, SSR_THREAD_GROUP_SIZE);
//
//                        RenderBackendShaderArguments shaderArguments = {};
//                        shaderArguments.BindBuffer(0, perFrameData.buffer, 0);
//                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneDepth)));
//                        shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer1)));
//                        shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(resolveVariance)));
//                        shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(temporalFilteringOutputTexture)));
//                        shaderArguments.BindTextureUAV(5, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(spatialFilteringIntermediateTexture), 0));
//                        shaderArguments.PushConstants(0, 1.0f);
//                        shaderArguments.PushConstants(1, 0.0f);
//
//                        auto spatialFilteringCS = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SSRSpatialFiltering);
//                        commandList.Dispatch2D(
//                            spatialFilteringCS,
//                            shaderArguments,
//                            dispatchX,
//                            dispatchY);
//                    };
//                });
//
//            renderGraph.AddPass("SSRSpatialFilteringVerticalPass", RenderGraphPassFlags::Compute,
//                [&](RenderGraphBuilder& builder)
//                {
//                    const auto& sceneDepthData = blackboard.Get<RealTimeRendererHistoryInfo>();
//                    const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//
//                    auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                    auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                    resolveVariance = builder.ReadTexture(resolveVariance, RenderBackendResourceState::ShaderResource);
//                    spatialFilteringIntermediateTexture = builder.ReadTexture(spatialFilteringIntermediateTexture, RenderBackendResourceState::ShaderResource);
//
//                    ssrTexture = builder.WriteTexture(ssrTexture, RenderBackendResourceState::UnorderedAccess);
//
//                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                    {
//                        uint32 dispatchX = Math::CeilDiv(view.targetWidth, SSR_THREAD_GROUP_SIZE);
//                        uint32 dispatchY = Math::CeilDiv(view.targetHeight, SSR_THREAD_GROUP_SIZE);
//
//                        RenderBackendShaderArguments shaderArguments = {};
//                        shaderArguments.BindBuffer(0, perFrameData.buffer, 0);
//                        shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(sceneDepth)));
//                        shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(gbuffer1)));
//                        shaderArguments.BindTextureSRV(3, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(resolveVariance)));
//                        shaderArguments.BindTextureSRV(4, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTexture(spatialFilteringIntermediateTexture)));
//                        shaderArguments.BindTextureUAV(5, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTexture(ssrTexture), 0));
//                        shaderArguments.PushConstants(0, 0.0f);
//                        shaderArguments.PushConstants(1, 1.0f);
//
//                        auto spatialFilteringCS = shaderLibrary->GetShaderHandle((uint32)RealTimeRendererShaderPiplineID::SSRSpatialFiltering);
//                        commandList.Dispatch2D(
//                            spatialFilteringCS,
//                            shaderArguments,
//                            dispatchX,
//                            dispatchY);
//                    };
//                });
//        }
//        else
//        {
//            ssrTexture = resolveTexture;
//        }
//    }
//}