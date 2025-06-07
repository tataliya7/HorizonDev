#include "SurfelBasedGlobalIllumination.h"
#include "RealTimeRenderer.h"

namespace Horizon
{
    bool RealTimeRenderer::IsSurfelGIEnabled() const
    {
        return features.enableSurfelGI;
    }

    //     void RealTimeRenderer::AddSurfleGIPasses(
//         RenderGraph& renderGraph,
//         const SceneView& view)
//     {
//         uint32 surfelGIRenderWidth = renderResolution.width;
//         uint32 surfelGIRenderHeight = renderResolution.height;
//
//         auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
//
//         if (view.frameIndex == 0)
//         {
//             /*renderGraph.AddPass(std::format("SurfelGIFreeSurfels"), RenderGraphPassFlags::Compute,
//                 [&](RenderGraphBuilder& builder)
//                 {
//                     return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                     {
//                         uint32 threadGroupCountX = ComputeWorkGroupCount(SurfelGIMaxSurfelCount, SurfelGIIndirectDispatchGroupThreadCount);
//
//                         RenderBackendShaderConstants shaderConstants = {};
//                         shaderConstants.BindBuffer(0, surfelGIInfoBuffer, 0);
//                         shaderConstants.BindBuffer(1, surfelGIFreeSurfelBuffer, 0);
//
//                         RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SurfelGIFreeSurfels);
//                         commandList.Dispatch(
//                             computeShader,
//                             shaderConstants,
//                             threadGroupCountX,
//                             1,
//                             1);
//                     };
//                 });*/
//         }
//
//         renderGraph.AddPass(std::format("SurfelGIGapFilling"), RenderGraphPassFlags::Compute,
//             [&](RenderGraphBuilder& builder)
//             {
//                 auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
//                 auto gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);
//
//                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                 {
//                     uint32 threadGroupCountX = ComputeWorkGroupCount(surfelGIRenderWidth, SurfelGIScreenTileSize);
//                     uint32 threadGroupCountY = ComputeWorkGroupCount(surfelGIRenderHeight, SurfelGIScreenTileSize);
//
//                     RenderBackendShaderConstants shaderConstants = {};
//                     shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
//                     shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture)));
//                     shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0)));
//                     shaderConstants.BindBuffer(3, surfelGIInfoBuffer, 0);
//                     shaderConstants.BindBuffer(4, surfelGICellHeaderBuffer, 0);
//                     shaderConstants.BindBuffer(5, surfelGICellDataBuffer, 0);
//                     shaderConstants.BindBuffer(6, surfelGIAliveSurfelIndirectionBuffer, 0);
//                     shaderConstants.BindBuffer(7, surfelGIFreeSurfelBuffer, 0);
//                     shaderConstants.BindBuffer(8, surfelGISurfelHotDataBuffer, 0);
//
//                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SurfelGIGapFilling);
//                     commandList.Dispatch(
//                         computeShader,
//                         shaderConstants,
//                         threadGroupCountX,
//                         threadGroupCountY,
//                         1);
//                 };
//             });
//
//         renderGraph.AddPass(std::format("SurfelGIGridReset (Compute, Cell Count: {})", SurfelGIUniformGridCellCount), RenderGraphPassFlags::Compute,
//             [&](RenderGraphBuilder& builder)
//             {
//                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                 {
//                     uint32 threadGroupCountX = ComputeWorkGroupCount(SurfelGIUniformGridCellCount, SurfelGIIndirectDispatchGroupThreadCount);
//
//                     RenderBackendShaderConstants shaderConstants = {};
//                     shaderConstants.BindBuffer(0, surfelGICellHeaderBuffer, 0);
//
//                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SurfelGIGridReset);
//                     commandList.Dispatch(
//                         computeShader,
//                         shaderConstants,
//                         threadGroupCountX,
//                         1,
//                         1);
//                 };
//             });
//
//         renderGraph.AddPass(std::format("SurfelGIComputeCellCapacity"), RenderGraphPassFlags::Compute,
//             [&](RenderGraphBuilder& builder)
//             {
//                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                 {
//                     RenderBackendShaderConstants shaderConstants = {};
//                     shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
//                     shaderConstants.BindBuffer(1, surfelGIInfoBuffer, 0);
//                     shaderConstants.BindBuffer(2, surfelGIAliveSurfelIndirectionBuffer, 0);
//                     shaderConstants.BindBuffer(3, surfelGISurfelHotDataBuffer, 0);
//                     shaderConstants.BindBuffer(4, surfelGICellHeaderBuffer, 0);
//
//                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SurfelGIComputeCellCapacity);
//                     commandList.DispatchIndirect(
//                         computeShader,
//                         shaderConstants,
//                         surfelGIArgumentBuffer,
//                         0);
//                 };
//             });
//
//         //renderGraph.AddPass(std::format("SurfelGIComputeCellOffset"), RenderGraphPassFlags::Compute,
//         //    [&](RenderGraphBuilder& builder)
//         //    {
//         //        return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//         //            {
//         //                uint32 threadGroupCountX = ComputeWorkGroupCount(SurfelGIUniformGridCellCount, 64);
//
//         //                RenderBackendShaderConstants shaderConstants = {};
//         //                shaderConstants.BindBuffer(0, surfelGICellHeaderBuffer, 0);
//
//         //                RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SurfelGIComputeCellOffset);
//         //                commandList.Dispatch(
//         //                    computeShader,
//         //                    shaderConstants,
//         //                    threadGroupCountX,
//         //                    1,
//         //                    1);
//         //            };
//         //    });
//
//         /*renderGraph.AddPass("SurfelGIIndirectArguments", RenderGraphPassFlags::Compute,
//             [&](RenderGraphBuilder& builder)
//             {
//                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                 {
//                     RenderBackendShaderConstants shaderConstants = {};
//                     shaderConstants.BindBuffer(0, surfelGIConstantsBuffer, 0);
//                     shaderConstants.BindBuffer(1, surfelGIIndirectArguments, 0);
//
//                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SurfelGIIndirectArguments);
//                     commandList.Dispatch(
//                         computeShader,
//                         shaderConstants,
//                         1,
//                         1,
//                         1);
//                 };
//             });*/
//     }
//
//     RenderGraphTextureHandle RealTimeRenderer::AddSurfleGIVisualizationPass(
//         RenderGraph& renderGraph,
//         const SceneView& view)
//     {
//         auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
//
//         RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(sceneTextures.sceneColorTextureDesc, "SurfelGIVisualizationTexture");
//
//         renderGraph.AddPass(std::format("SurfelGIVisualization (Compute, {}x{})", renderResolution.width, renderResolution.height), RenderGraphPassFlags::Compute,
//             [&](RenderGraphBuilder& builder)
//             {
//                 auto sceneColorTexture = builder.ReadTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::ShaderResource);
//                 auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
//
//                 outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);
//
//                 return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
//                 {
//                     uint32 threadGroupCountX = ComputeWorkGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
//                     uint32 threadGroupCountY = ComputeWorkGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
//
//                     RenderBackendShaderConstants shaderConstants = {};
//                     shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
//                     shaderConstants.BindTextureSRV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneColorTexture)));
//                     shaderConstants.BindTextureSRV(2, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture)));
//                     shaderConstants.BindBuffer(3, surfelGIInfoBuffer, 0);
//                     shaderConstants.BindBuffer(4, surfelGICellHeaderBuffer, 0);
//                     shaderConstants.BindBuffer(5, surfelGICellDataBuffer, 0);
//                     shaderConstants.BindBuffer(6, surfelGISurfelHotDataBuffer, 0);
//                     shaderConstants.BindTextureUAV(7, registry.GetTextureUAVBindlessResourceDescriptorIndexoutputTexture), 0));
//
//                     RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::SurfelGIVisualization);
//                     commandList.Dispatch2D(
//                         computeShader,
//                         shaderConstants,
//                         threadGroupCountX,
//                         groupCountY);
//                 };
//             });
//
//         sceneTextures.sceneColorTexture = outputTexture;
//
//         return outputTexture;
//     }
}