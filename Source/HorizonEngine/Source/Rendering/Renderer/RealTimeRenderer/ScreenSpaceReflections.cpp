#include "RealTimeRenderer.h"

#define SSR_THREAD_GROUP_SIZE 8

namespace Horizon
{
    static constexpr uint32 GScreenSpaceReflectionsThreadGroupSizeX = 8;
    static constexpr uint32 GScreenSpaceReflectionsThreadGroupSizeY = 8;
    static constexpr uint32 GScreenSpaceReflectionsTileSize = 32;

    bool RealTimeRenderer::IsScreenSpaceReflectionsEnabled() const
    {
        return features.enableScreenSpaceReflections;
    }

    void RealTimeRenderer::RenderScreenSpaceReflections(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
//         RenderGraphTextureHandle ssrInputColorTexture = sceneColorTexture;
//         if ()
//         {
//             RenderGraphTextureHandle previousTemporalSuperSamplingTexture = renderGraph.ImportExternalTexture();
//             ssrInputColorTexture = previousTemporalSuperSamplingTexture;
//         }
//
//         uint32 deviceMask = ~0u;
//         const auto& perFrameData = blackboard.Get<RealTimeRendererSceneViewInfo>();
//

#if 0
         const uint32 tileCountX = CeilDiv(renderResolution.width, GScreenSpaceReflectionsTileSize);
         const uint32 tileCountY = CeilDiv(renderResolution.height, GScreenSpaceReflectionsTileSize);

         RenderGraphTextureHandle tileClassificationHorizontalTexture = renderGraph.CreateTexture(
             RenderGraphTextureDesc::Create2D(
                 tileCountX,
                 view.targetHeight,
                 RenderBackendTextureFormat::R16G16Float,
                 RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
             "SSRTileClassificationHorizontalTexture");

         RenderGraphTextureHandle tileClassificationTexture = renderGraph.CreateTexture(
             RenderGraphTextureDesc::Create2D(
                 tileCountX,
                 tileCountY,
                 RenderBackendTextureFormat::R16G16Float,
                 RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
             "SSRTileClassificationTexture");

        RenderGraphBufferDesc rayAllocationBufferDesc = RenderGraphBufferDesc::CreateIndirectArguments(sizeof(uint32), 12);
        RenderGraphBufferHandle rayAllocationBuffer = renderGraph.CreateBuffer(rayAllocationBufferDesc, "SSRRayAllocationBuffer");

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

    RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

    renderGraph.AddPass(
        "SSRTileClassificationHorizontalPass",
        RenderGraphPassFlags::Compute,
        [&](RenderGraphBuilder& builder)
        {
            RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
            RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
            tileClassificationHorizontalTexture = builder.WriteTexture(tileClassificationHorizontalTexture, RenderBackendResourceState::UnorderedAccess);

            return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
            {
                uint32 threadGroupCountX = CeilDiv(tileCountX, GScreenSpaceReflectionsThreadGroupSizeX);
                uint32 threadGroupCountY = CeilDiv(view.targetHeight, SSR_THREAD_GROUP_SIZE);
                uint32 threadGroupCountZ = 1;

                RenderBackendShaderConstants shaderConstants = {};
                shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                shaderConstants.BindTextureUAV(3, registry.GetTextureUAVBindlessResourceDescriptorIndex(tileClassificationHorizontalTexture, 0));

                RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SSRTileClassificationHorizontal);

                commandList.Dispatch(
                    computeShader,
                    shaderConstants,
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
            RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
            RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
            tileClassificationHorizontalTexture = builder.ReadTexture(tileClassificationHorizontalTexture, RenderBackendResourceState::ShaderResource);
            tileClassificationTexture = builder.WriteTexture(tileClassificationTexture, RenderBackendResourceState::UnorderedAccess);
            rayAllocationBuffer = builder.WriteBuffer(rayAllocationBuffer, RenderBackendResourceState::UnorderedAccess);
            // earlyExitTilesBuffer = builder.WriteTexture(earlyExitTilesBuffer, RenderBackendResourceState::UnorderedAccess);
            // cheapTilesBuffer = builder.WriteTexture(cheapTilesBuffer, RenderBackendResourceState::UnorderedAccess);
            // expensiveTilesBuffer = builder.WriteTexture(expensiveTilesBuffer, RenderBackendResourceState::UnorderedAccess);

            return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
            {
                uint32 threadGroupCountX = CeilDiv(tileCountX, GScreenSpaceReflectionsThreadGroupSizeX);
                uint32 threadGroupCountY = CeilDiv(tileCountY, SSR_THREAD_GROUP_SIZE);
                uint32 threadGroupCountZ = 1;

                RenderBackendShaderConstants shaderConstants = {};
                shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(tileClassificationHorizontalTexture));
                shaderConstants.BindTextureUAV(4, registry.GetTextureUAVBindlessResourceDescriptorIndex(tileClassificationTexture, 0));
                shaderConstants.BindBufferUAV(5, registry.GetBufferUAVBindlessResourceDescriptorIndex(rayAllocationBuffer));
                // shaderConstants.BindTextureUAV(5, registry.GetTextureUAVBindlessResourceDescriptorIndexearlyExitTilesBuffer), 0));
                // shaderConstants.BindTextureUAV(6, registry.GetTextureUAVBindlessResourceDescriptorIndexcheapTilesBuffer), 0));
                // shaderConstants.BindTextureUAV(7, registry.GetTextureUAVBindlessResourceDescriptorIndexexpensiveTilesBuffer), 0));

                RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SSRTileClassificationVertical);

                commandList.Dispatch(
                    computeShader,
                    shaderConstants,
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
            rayAllocationBuffer = builder.WriteBuffer(rayAllocationBuffer, RenderBackendResourceState::UnorderedAccess);

            return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
            {
                RenderBackendShaderConstants shaderConstants = {};
                shaderConstants.BindBufferUAV(0, registry.GetBufferUAVBindlessResourceDescriptorIndex(rayAllocationBuffer));

               RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SSRRayAllocation);

                commandList.Dispatch(
                    computeShader,
                    shaderConstants,
                    1,
                    1,
                    1);
            };
        });

        RenderGraphTextureHandle rayIndirectSpecular = renderGraph.CreateTexture(
            RenderGraphTextureDesc::Create2D(
                renderResolution.width,
                renderResolution.height,
                RenderBackendTextureFormat::R16G16B16A16Float,
                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
            "SSRRayIndirectSpecular");

        RenderGraphTextureHandle rayDirectionAndLengthTexture = renderGraph.CreateTexture(
            RenderGraphTextureDesc::Create2D(
                renderResolution.width,
                renderResolution.height,
                RenderBackendTextureFormat::R16G16B16A16Float,
                RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
            "SSRRayDirectionAndLength");
//
//         RenderGraphTextureDesc rayLengthTextureDesc = RenderGraphTextureDesc::Create2D(
//             rayCastingResolutionX,
//             rayCastingResolutionY,
//             RenderBackendTextureFormat::R16Float,
//             RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource);
//         RenderGraphTextureHandle rayLengthTexture = renderGraph.CreateTexture(rayLengthTextureDesc, "SSRRayLengthTexture");
//
//         const Vector2f hzbUVFactor = Vector2f(
//             float(perFrameData.paramaters.renderResolution.width) / float(hzbWidth),
//             float(perFrameData.paramaters.renderResolution.height) / float(hzbHeight));
//         Vector4f hzbUVFactorAndInvFactor = Vector4f(hzbUVFactor.x, hzbUVFactor.y, 1.0f / hzbUVFactor.x, 1.0 / hzbUVFactor.y);

        RenderGraphTextureHandle previousSceneColorTexture = defaultResources->ImportBlackDummyTexture2D(renderGraph);
        if (historyFrame.temporalSuperSamplingOutputTexture)
        {
            previousSceneColorTexture = renderGraph.ImportExternalTexture(historyFrame.temporalSuperSamplingOutputTexture, "SSRInputColorTexture");
        }

        renderGraph.AddPass(
            "SSRRayTracingPass-EarlyExit (Compute, Indirect)",
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle minDepthPyramidTexture = builder.ReadTexture(sceneTextures.minDepthPyramidTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle motionVectorTexture = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);
                previousSceneColorTexture = builder.ReadTexture(previousSceneColorTexture, RenderBackendResourceState::ShaderResource);
                // earlyExitTilesBuffer = builder.ReadTexture(earlyExitTilesBuffer, RenderBackendResourceState::ShaderResource);
                // cheapTilesBuffer = builder.ReadTexture(cheapTilesBuffer, RenderBackendResourceState::ShaderResource);
                // expensiveTilesBuffer = builder.ReadTexture(expensiveTilesBuffer, RenderBackendResourceState::ShaderResource);
                // rayIndirectSpecular = builder.WriteTexture(rayIndirectSpecular, RenderBackendResourceState::UnorderedAccess);
                rayDirectionAndLengthTexture = builder.WriteTexture(rayDirectionAndLengthTexture, RenderBackendResourceState::UnorderedAccess);
                // rayLength = builder.WriteTexture(rayLength, RenderBackendResourceState::UnorderedAccess);
                // debugOutputTexture = builder.WriteTexture(debugOutputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(minDepthPyramidTexture));
                    shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(motionVectorTexture));
                    shaderConstants.BindTextureSRV(6, registry.GetTextureSRVBindlessResourceDescriptorIndex(previousSceneColorTexture));
                    shaderConstants.BindTextureUAV(7, registry.GetTextureUAVBindlessResourceDescriptorIndex(rayIndirectSpecular, 0));
                    shaderConstants.BindTextureUAV(8, registry.GetTextureUAVBindlessResourceDescriptorIndex(rayDirectionAndLengthTexture), 0);
                    shaderConstants.BindTextureUAV(9, registry.GetTextureUAVBindlessResourceDescriptorIndex(rayLength), 0));
                    shaderConstants.PushConstants(0, float(rayCastingResolutionX));
                    shaderConstants.PushConstants(1, float(rayCastingResolutionY));
                    shaderConstants.PushConstants(2, hzbUVFactorAndInvFactor.x);
                    shaderConstants.PushConstants(3, hzbUVFactorAndInvFactor.y);
                    shaderConstants.PushConstants(4, hzbUVFactorAndInvFactor.z);
                    shaderConstants.PushConstants(5, hzbUVFactorAndInvFactor.w);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SSRRayTracingEarlyExit);

                    commandList.DispatchIndirect(
                        computeShader,
                        shaderConstants,
                        registry.GetRenderBackendBufferHandle(rayAllocationBuffer),
                        24);
                };
            });

        renderGraph.AddPass(
            "SSRRayTracingPass-Cheap (Compute, Indirect)",
            RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                RenderGraphTextureHandle sceneDepth = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle gbuffer1 = builder.ReadTexture(sceneTextures.gbuffer1, RenderBackendResourceState::ShaderResource);
                RenderGraphTextureHandle motionVectors = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);
                // earlyExitTilesBuffer = builder.ReadTexture(earlyExitTilesBuffer, RenderBackendResourceState::ShaderResource);
                // cheapTilesBuffer = builder.ReadTexture(cheapTilesBuffer, RenderBackendResourceState::ShaderResource);
                // expensiveTilesBuffer = builder.ReadTexture(expensiveTilesBuffer, RenderBackendResourceState::ShaderResource);
                // rayIndirectSpecular = builder.WriteTexture(rayIndirectSpecular, RenderBackendResourceState::UnorderedAccess);
                // rayDirectionPDF = builder.WriteTexture(rayDirectionPDF, RenderBackendResourceState::UnorderedAccess);
                // rayLength = builder.WriteTexture(rayLength, RenderBackendResourceState::UnorderedAccess);
                // debugOutputTexture = builder.WriteTexture(debugOutputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
                    shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1));
                    shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(motionVectors));
                    shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepth));
                    shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(hzb));
                    shaderConstants.BindTextureSRV(6, registry.GetTextureSRVBindlessResourceDescriptorIndex(historySceneColor));
                    shaderConstants.BindTextureUAV(7, registry.GetTextureUAVBindlessResourceDescriptorIndex(rayIndirectSpecular), 0);
                    shaderConstants.BindTextureUAV(8, registry.GetTextureUAVBindlessResourceDescriptorIndex(rayDirectionPDF), 0);
                    shaderConstants.BindTextureUAV(9, registry.GetTextureUAVBindlessResourceDescriptorIndex(rayLength), 0);
                    shaderConstants.BindTextureSRV(10, RenderBackendTextureSRVDesc::Create(blueNoiseTexture));
                    shaderConstants.BindTextureUAV(15, registry.GetTextureUAVBindlessResourceDescriptorIndex(debugOutputTexture, 0));
                    shaderConstants.PushConstants(0, float(rayCastingResolutionX));
                    shaderConstants.PushConstants(1, float(rayCastingResolutionY));
                    shaderConstants.PushConstants(2, hzbUVFactorAndInvFactor.x);
                    shaderConstants.PushConstants(3, hzbUVFactorAndInvFactor.y);
                    shaderConstants.PushConstants(4, hzbUVFactorAndInvFactor.z);
                    shaderConstants.PushConstants(5, hzbUVFactorAndInvFactor.w);

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SSRRayTracingCheap);

                    commandList.DispatchIndirect(
                        computeShader,
                        shaderConstants,
                        registry.GetRenderBackendBufferHandle(rayAllocationBuffer),
                        24);
                };
            });
#endif
//
//         RenderGraphTextureHandle resolveTexture = renderGraph.CreateTexture(
//             RenderGraphTextureDesc::Create2D(
//                 view.targetWidth,
//                 view.targetHeight,
//                 RenderBackendTextureFormat::RGBA16Float,
//                 RenderBackendTextureCreateFlags::UnorderedAccess | RenderBackendTextureCreateFlags::ShaderResource),
//             "SSRResolveTexture");
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
//
//         renderGraph.AddPass("SSRColorResolvePass", RenderGraphPassFlags::Compute,
//             [&](RenderGraphBuilder& builder)
//             {
//                 const auto& sceneDepthData = blackboard.Get<RealTimeRendererHistoryInfo>();
//                 const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//
//                 auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                 auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                 rayIndirectSpecular = builder.ReadTexture(rayIndirectSpecular, RenderBackendResourceState::ShaderResource);
//                 rayDirectionPDF = builder.ReadTexture(rayDirectionPDF, RenderBackendResourceState::ShaderResource);
//                 rayLength = builder.ReadTexture(rayLength, RenderBackendResourceState::ShaderResource);
//
//                 resolveTexture = builder.WriteTexture(resolveTexture, RenderBackendResourceState::UnorderedAccess);
//                 resolveVariance = builder.WriteTexture(resolveVariance, RenderBackendResourceState::UnorderedAccess);
//                 reprojectionDepth = builder.WriteTexture(reprojectionDepth, RenderBackendResourceState::UnorderedAccess);
//
//                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                 {
//                     uint32 threadGroupCountX = ComputeWorkGroupCount(view.targetWidth, SSR_THREAD_GROUP_SIZE);
//                     uint32 threadGroupCountY = ComputeWorkGroupCount(view.targetHeight, SSR_THREAD_GROUP_SIZE);
//
//                     RenderBackendShaderConstants shaderConstants = {};
//                     shaderConstants.BindBuffer(0, perFrameData.buffer, 0);
//                     shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepth)));
//                     shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1)));
//                     shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(rayIndirectSpecular)));
//                     shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(rayDirectionPDF)));
//                     shaderConstants.BindTextureSRV(5, registry.GetTextureSRVBindlessResourceDescriptorIndex(rayLength)));
//                     shaderConstants.BindTextureUAV(6, registry.GetTextureUAVBindlessResourceDescriptorIndexresolveTexture), 0));
//                     shaderConstants.BindTextureUAV(7, registry.GetTextureUAVBindlessResourceDescriptorIndexresolveVariance), 0));
//                     shaderConstants.BindTextureUAV(8, registry.GetTextureUAVBindlessResourceDescriptorIndexreprojectionDepth), 0));
//
//                     auto resolveCS = shaderLibrary->GetShader(ShaderID::SSRColorResolve);
//                     commandList.Dispatch2D(
//                         resolveCS,
//                         shaderConstants,
//                         threadGroupCountX,
//                         groupCountY);
//                 };
//             });
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
//                     const auto& sceneDepthData = blackboard.Get<RealTimeRendererHistoryInfo>();
//                     auto& sceneTextures = blackboard.Get<RealTimeRendererSceneTextures>();
//
//                     auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                     auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                     auto motionVectors = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);
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
//                     return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                     {
//                         uint32 threadGroupCountX = ComputeWorkGroupCount(view.targetWidth, SSR_THREAD_GROUP_SIZE);
//                         uint32 threadGroupCountY = ComputeWorkGroupCount(view.targetHeight, SSR_THREAD_GROUP_SIZE);
//
//                         RenderBackendShaderConstants shaderConstants = {};
//                         shaderConstants.BindBuffer(0, perFrameData.buffer, 0);
//                         shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepth)));
//                         shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1)));
//                         shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(motionVectors)));
//                         shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(resolveTexture)));
//                         shaderConstants.BindTextureSRV(5, registry.GetTextureSRVBindlessResourceDescriptorIndex(resolveVariance)));
//                         shaderConstants.BindTextureSRV(6, registry.GetTextureSRVBindlessResourceDescriptorIndex(reprojectionDepth)));
//                         shaderConstants.BindTextureSRV(7, registry.GetTextureSRVBindlessResourceDescriptorIndex(historyTemporalFilteringOutputTexture)));
//                         shaderConstants.BindTextureSRV(8, registry.GetTextureSRVBindlessResourceDescriptorIndex(historyTemporalVarianceTexture)));
//                         shaderConstants.BindTextureSRV(9, registry.GetTextureSRVBindlessResourceDescriptorIndex(historySceneDepth)));
//                         shaderConstants.BindTextureUAV(10, registry.GetTextureUAVBindlessResourceDescriptorIndextemporalFilteringOutputTexture), 0));
//                         shaderConstants.BindTextureUAV(11, registry.GetTextureUAVBindlessResourceDescriptorIndextemporalVarianceTexture), 0));
//
//                         auto temporalFilteringCS = shaderLibrary->GetShader(ShaderID::SSRTemporalFiltering);
//                         commandList.Dispatch2D(
//                             temporalFilteringCS,
//                             shaderConstants,
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
//                     const auto& sceneDepthData = blackboard.Get<RealTimeRendererHistoryInfo>();
//                     const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//
//                     auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                     auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                     resolveVariance = builder.ReadTexture(resolveVariance, RenderBackendResourceState::ShaderResource);
//                     temporalFilteringOutputTexture = builder.ReadTexture(temporalFilteringOutputTexture, RenderBackendResourceState::ShaderResource);
//
//                     spatialFilteringIntermediateTexture = builder.WriteTexture(spatialFilteringIntermediateTexture, RenderBackendResourceState::UnorderedAccess);
//
//                     return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                     {
//                         uint32 threadGroupCountX = ComputeWorkGroupCount(view.targetWidth, SSR_THREAD_GROUP_SIZE);
//                         uint32 threadGroupCountY = ComputeWorkGroupCount(view.targetHeight, SSR_THREAD_GROUP_SIZE);
//
//                         RenderBackendShaderConstants shaderConstants = {};
//                         shaderConstants.BindBuffer(0, perFrameData.buffer, 0);
//                         shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepth)));
//                         shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1)));
//                         shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(resolveVariance)));
//                         shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(temporalFilteringOutputTexture)));
//                         shaderConstants.BindTextureUAV(5, registry.GetTextureUAVBindlessResourceDescriptorIndexspatialFilteringIntermediateTexture), 0));
//                         shaderConstants.PushConstants(0, 1.0f);
//                         shaderConstants.PushConstants(1, 0.0f);
//
//                         auto spatialFilteringCS = shaderLibrary->GetShader(ShaderID::SSRSpatialFiltering);
//                         commandList.Dispatch2D(
//                             spatialFilteringCS,
//                             shaderConstants,
//                             threadGroupCountX,
//                             groupCountY);
//                     };
//                 });
//
//             renderGraph.AddPass("SSRSpatialFilteringVerticalPass", RenderGraphPassFlags::Compute,
//                 [&](RenderGraphBuilder& builder)
//                 {
//                     const auto& sceneDepthData = blackboard.Get<RealTimeRendererHistoryInfo>();
//                     const auto& gbufferData = blackboard.Get<RenderGraphGBuffer>();
//
//                     auto sceneDepth = builder.ReadTexture(sceneDepthData.sceneDepth, RenderBackendResourceState::ShaderResource);
//                     auto gbuffer1 = builder.ReadTexture(gbufferData.gbuffer1, RenderBackendResourceState::ShaderResource);
//                     resolveVariance = builder.ReadTexture(resolveVariance, RenderBackendResourceState::ShaderResource);
//                     spatialFilteringIntermediateTexture = builder.ReadTexture(spatialFilteringIntermediateTexture, RenderBackendResourceState::ShaderResource);
//
//                     ssrTexture = builder.WriteTexture(ssrTexture, RenderBackendResourceState::UnorderedAccess);
//
//                     return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                     {
//                         uint32 threadGroupCountX = ComputeWorkGroupCount(view.targetWidth, SSR_THREAD_GROUP_SIZE);
//                         uint32 threadGroupCountY = ComputeWorkGroupCount(view.targetHeight, SSR_THREAD_GROUP_SIZE);
//
//                         RenderBackendShaderConstants shaderConstants = {};
//                         shaderConstants.BindBuffer(0, perFrameData.buffer, 0);
//                         shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepth)));
//                         shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer1)));
//                         shaderConstants.BindTextureSRV(3, registry.GetTextureSRVBindlessResourceDescriptorIndex(resolveVariance)));
//                         shaderConstants.BindTextureSRV(4, registry.GetTextureSRVBindlessResourceDescriptorIndex(spatialFilteringIntermediateTexture)));
//                         shaderConstants.BindTextureUAV(5, registry.GetTextureUAVBindlessResourceDescriptorIndexssrTexture), 0));
//                         shaderConstants.PushConstants(0, 0.0f);
//                         shaderConstants.PushConstants(1, 1.0f);
//
//                         auto spatialFilteringCS = shaderLibrary->GetShader(ShaderID::SSRSpatialFiltering);
//                         commandList.Dispatch2D(
//                             spatialFilteringCS,
//                             shaderConstants,
//                             threadGroupCountX,
//                             groupCountY);
//                     };
//                 });
//         }
//         else
//         {
//             ssrTexture = resolveTexture;
//         }
    }
}