#include "SurfelBasedGlobalIllumination.h"
#include "RasterizationRenderer.h"

namespace Horizon
{
    // Surfel hot data
    struct SurfelHotData
    {
        Vector3f position;
        Vector3f normal;
        float radius;
    };

    // Surfel cold data
    struct SurfelColdData
    {
        Vector3f position;
        Vector3f normal;
        Vector2i primitiveID;
    };

    struct CellHeader
    {
        uint32 capacity;
        uint32 offset;
    };

    static const uint32 SurfelGIIndirectDispatchGroupThreadCount = 64;
    static const uint32 SurfelGIMaxSurfelCount = 100000;
    static const uint32 SurfelGIMaxSurfelCountPerCell = 10;
    static const uint32 SurfelGIUniformGridCellSize = 2;
    static const Vector3i SurfelGIUniformGridSize = Vector3i(128, 128, 64); // The size of each dimension must be a multiple of 2!
    static const uint32 SurfelGIUniformGridCellCount = SurfelGIUniformGridSize.x * SurfelGIUniformGridSize.y * SurfelGIUniformGridSize.z;
    static const float SurfelGICoverageThreshold = 0.5f;
    static const uint32 SurfelGIScreenTileSize = 16;
    static const uint32 SurfelGIInfoBufferOffset_AliveSurfelCount = 0;
    static const uint32 SurfelGIInfoBufferOffset_DeadSurfelCount = SurfelGIInfoBufferOffset_AliveSurfelCount + 4;
    static const uint32 SurfelGIInfoBufferSize = SurfelGIInfoBufferOffset_DeadSurfelCount + 4;

    bool RasterizationRenderer::IsSurfelGIEnabled() const
    {
        return renderFeatures.enableSurfelGI;
    }

//     void RasterizationRenderer::AddSurfleGIPasses(
//         RenderGraph& renderGraph,
//         const SceneView& view)
//     {
//         uint32 surfelGIRenderWidth = renderResolution.width;
//         uint32 surfelGIRenderHeight = renderResolution.height;
//
//         auto& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererSceneTextures>();
//
//         if (view.frameIndex == 0)
//         {
//             /*renderGraph.AddPass(std::format("SurfelGIFreeSurfels"), RenderGraphPassFlags::Compute,
//                 [&](RenderGraphBuilder& builder)
//                 {
//
//                     {
//                         uint32 threadGroupCountX = ComputeWorkGroupCount(SurfelGIMaxSurfelCount, SurfelGIIndirectDispatchGroupThreadCount);
//
//                         RenderBackendPushConstantValues pushConstantValues = {};
//                         pushConstantValues.BindBuffer(0, surfelGIInfoBuffer, 0);
//                         pushConstantValues.BindBuffer(1, surfelGIFreeSurfelBuffer, 0);
//
//                         RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SurfelGIFreeSurfels);
//                         commandList.Dispatch(
//                             computeShader,
//                             pushConstantValues,
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
//                 auto sceneDepthTexture = builder.ReadTexture(intermediateResources.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
//                 auto gbuffer0 = builder.ReadTexture(intermediateResources.gbuffer0, RenderBackendResourceState::ShaderResource);
//
//
//                 {
//                     uint32 threadGroupCountX = ComputeWorkGroupCount(surfelGIRenderWidth, SurfelGIScreenTileSize);
//                     uint32 threadGroupCountY = ComputeWorkGroupCount(surfelGIRenderHeight, SurfelGIScreenTileSize);
//
//                     RenderBackendPushConstantValues pushConstantValues = {};
//                     pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
//                     pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture)));
//                     pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(gbuffer0)));
//                     pushConstantValues.BindBuffer(3, surfelGIInfoBuffer, 0);
//                     pushConstantValues.BindBuffer(4, surfelGICellHeaderBuffer, 0);
//                     pushConstantValues.BindBuffer(5, surfelGICellDataBuffer, 0);
//                     pushConstantValues.BindBuffer(6, surfelGIAliveSurfelIndirectionBuffer, 0);
//                     pushConstantValues.BindBuffer(7, surfelGIFreeSurfelBuffer, 0);
//                     pushConstantValues.BindBuffer(8, surfelGISurfelHotDataBuffer, 0);
//
//                     RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SurfelGIGapFilling);
//                     commandList.Dispatch(
//                         computeShader,
//                         pushConstantValues,
//                         threadGroupCountX,
//                         threadGroupCountY,
//                         1);
//                 };
//             });
//
//         renderGraph.AddPass(std::format("SurfelGIGridReset (Compute, Cell Count: {})", SurfelGIUniformGridCellCount), RenderGraphPassFlags::Compute,
//             [&](RenderGraphBuilder& builder)
//             {
//
//                 {
//                     uint32 threadGroupCountX = ComputeWorkGroupCount(SurfelGIUniformGridCellCount, SurfelGIIndirectDispatchGroupThreadCount);
//
//                     RenderBackendPushConstantValues pushConstantValues = {};
//                     pushConstantValues.BindBuffer(0, surfelGICellHeaderBuffer, 0);
//
//                     RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SurfelGIGridReset);
//                     commandList.Dispatch(
//                         computeShader,
//                         pushConstantValues,
//                         threadGroupCountX,
//                         1,
//                         1);
//                 };
//             });
//
//         renderGraph.AddPass(std::format("SurfelGIComputeCellCapacity"), RenderGraphPassFlags::Compute,
//             [&](RenderGraphBuilder& builder)
//             {
//
//                 {
//                     RenderBackendPushConstantValues pushConstantValues = {};
//                     pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
//                     pushConstantValues.BindBuffer(1, surfelGIInfoBuffer, 0);
//                     pushConstantValues.BindBuffer(2, surfelGIAliveSurfelIndirectionBuffer, 0);
//                     pushConstantValues.BindBuffer(3, surfelGISurfelHotDataBuffer, 0);
//                     pushConstantValues.BindBuffer(4, surfelGICellHeaderBuffer, 0);
//
//                     RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SurfelGIComputeCellCapacity);
//                     commandList.DispatchIndirect(
//                         computeShader,
//                         pushConstantValues,
//                         surfelGIArgumentBuffer,
//                         0);
//                 };
//             });
//
//         //renderGraph.AddPass(std::format("SurfelGIComputeCellOffset"), RenderGraphPassFlags::Compute,
//         //    [&](RenderGraphBuilder& builder)
//         //    {
//         //
//         //            {
//         //                uint32 threadGroupCountX = ComputeWorkGroupCount(SurfelGIUniformGridCellCount, 64);
//
//         //                RenderBackendPushConstantValues pushConstantValues = {};
//         //                pushConstantValues.BindBuffer(0, surfelGICellHeaderBuffer, 0);
//
//         //                RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SurfelGIComputeCellOffset);
//         //                commandList.Dispatch(
//         //                    computeShader,
//         //                    pushConstantValues,
//         //                    threadGroupCountX,
//         //                    1,
//         //                    1);
//         //            };
//         //    });
//
//         /*renderGraph.AddPass("SurfelGIIndirectArguments", RenderGraphPassFlags::Compute,
//             [&](RenderGraphBuilder& builder)
//             {
//
//                 {
//                     RenderBackendPushConstantValues pushConstantValues = {};
//                     pushConstantValues.BindBuffer(0, surfelGIConstantsBuffer, 0);
//                     pushConstantValues.BindBuffer(1, surfelGIIndirectArguments, 0);
//
//                     RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SurfelGIIndirectArguments);
//                     commandList.Dispatch(
//                         computeShader,
//                         pushConstantValues,
//                         1,
//                         1,
//                         1);
//                 };
//             });*/
//     }
//
//     RenderGraphTextureHandle RasterizationRenderer::AddSurfleGIVisualizationPass(
//         RenderGraph& renderGraph,
//         const SceneView& view)
//     {
//         auto& intermediateResources = renderGraph.blackboard.Get<RasterizationRendererSceneTextures>();
//
//         RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(intermediateResources.colorTextureDescription, "SurfelGIVisualizationTexture");
//
//         renderGraph.AddPass(std::format("SurfelGIVisualization (Compute, {}x{})", renderResolution.width, renderResolution.height), RenderGraphPassFlags::Compute,
//             [&](RenderGraphBuilder& builder)
//             {
//                 auto sceneColorTexture = builder.ReadTexture(intermediateResources.sceneColorTexture, RenderBackendResourceState::ShaderResource);
//                 auto sceneDepthTexture = builder.ReadTexture(intermediateResources.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
//
//                 outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);
//
//
//                 {
//                     uint32 threadGroupCountX = ComputeWorkGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
//                     uint32 threadGroupCountY = ComputeWorkGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
//
//                     RenderBackendPushConstantValues pushConstantValues = {};
//                     pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(GetCurrentPerFrameConstantBuffer()));
//                     pushConstantValues.BindTextureSRV(1, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneColorTexture)));
//                     pushConstantValues.BindTextureSRV(2, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture)));
//                     pushConstantValues.BindBuffer(3, surfelGIInfoBuffer, 0);
//                     pushConstantValues.BindBuffer(4, surfelGICellHeaderBuffer, 0);
//                     pushConstantValues.BindBuffer(5, surfelGICellDataBuffer, 0);
//                     pushConstantValues.BindBuffer(6, surfelGISurfelHotDataBuffer, 0);
//                     pushConstantValues.BindTextureUAV(7, resourceRegistry.GetTextureUAVBindlessResourceDescriptorIndexoutputTexture), 0));
//
//                     RenderBackendShaderHandle computeShader = shaderRepository->GetShader(ShaderID::SurfelGIVisualization);
//                     commandList.Dispatch2D(
//                         computeShader,
//                         pushConstantValues,
//                         threadGroupCountX,
//                         groupCountY);
//                 };
//             });
//
//         intermediateResources.sceneColorTexture = outputTexture;
//
//         return outputTexture;
//     }
}