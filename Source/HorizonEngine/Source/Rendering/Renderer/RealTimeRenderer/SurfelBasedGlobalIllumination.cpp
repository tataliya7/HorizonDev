#include "RealTimeRenderer.h"

namespace Horizon
{
    void RealTimeRenderer::AddSurfleGIPasses(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        uint32 surfelGIRenderWidth = renderResolution.width;
        uint32 surfelGIRenderHeight = renderResolution.height;

        auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        if (view.frameIndex == 0)
        {
            /*renderGraph.AddPass(std::format("SurfelGIFreeSurfels"), RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 groupCountX = ComputeWorkGroupCount(SurfelGIMaxSurfelCount, SurfelGIIndirectDispatchGroupThreadCount);

                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, surfelGIInfoBuffer, 0);
                        shaderArguments.BindBuffer(1, surfelGIFreeSurfelBuffer, 0);

                        RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::SurfelGIFreeSurfels);
                        commandList.Dispatch(
                            computeShader,
                            shaderArguments,
                            groupCountX,
                            1,
                            1);
                    };
                });*/
        }

        renderGraph.AddPass(std::format("SurfelGIGapFilling"), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                auto gbuffer0 = builder.ReadTexture(sceneTextures.gbuffer0, RenderBackendResourceState::ShaderResource);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(surfelGIRenderWidth, SurfelGIScreenTileSize);
                    uint32 groupCountY = ComputeWorkGroupCount(surfelGIRenderHeight, SurfelGIScreenTileSize);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(gbuffer0)));
                    shaderArguments.BindBuffer(3, surfelGIInfoBuffer, 0);
                    shaderArguments.BindBuffer(4, surfelGICellHeaderBuffer, 0);
                    shaderArguments.BindBuffer(5, surfelGICellDataBuffer, 0);
                    shaderArguments.BindBuffer(6, surfelGIAliveSurfelIndirectionBuffer, 0);
                    shaderArguments.BindBuffer(7, surfelGIFreeSurfelBuffer, 0);
                    shaderArguments.BindBuffer(8, surfelGISurfelHotDataBuffer, 0);

                    RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::SurfelGIGapFilling);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY,
                        1);
                };
            });

        renderGraph.AddPass(std::format("SurfelGIGridReset (Compute, Cell Count: {})", SurfelGIUniformGridCellCount), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(SurfelGIUniformGridCellCount, SurfelGIIndirectDispatchGroupThreadCount);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, surfelGICellHeaderBuffer, 0);

                    RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::SurfelGIGridReset);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        1,
                        1);
                };
            });

        renderGraph.AddPass(std::format("SurfelGIComputeCellCapacity"), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindBuffer(1, surfelGIInfoBuffer, 0);
                    shaderArguments.BindBuffer(2, surfelGIAliveSurfelIndirectionBuffer, 0);
                    shaderArguments.BindBuffer(3, surfelGISurfelHotDataBuffer, 0);
                    shaderArguments.BindBuffer(4, surfelGICellHeaderBuffer, 0);

                    RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::SurfelGIComputeCellCapacity);
                    commandList.DispatchIndirect(
                        computeShader,
                        shaderArguments,
                        surfelGIArgumentBuffer,
                        0);
                };
            });

        //renderGraph.AddPass(std::format("SurfelGIComputeCellOffset"), RenderGraphPassFlags::Compute,
        //    [&](RenderGraphBuilder& builder)
        //    {
        //        return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
        //            {
        //                uint32 groupCountX = ComputeWorkGroupCount(SurfelGIUniformGridCellCount, 64);

        //                RenderBackendShaderArguments shaderArguments = {};
        //                shaderArguments.BindBuffer(0, surfelGICellHeaderBuffer, 0);

        //                RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::SurfelGIComputeCellOffset);
        //                commandList.Dispatch(
        //                    computeShader,
        //                    shaderArguments,
        //                    groupCountX,
        //                    1,
        //                    1);
        //            };
        //    });

        /*renderGraph.AddPass("SurfelGIIndirectAruguments", RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, surfelGIConstantsBuffer, 0);
                    shaderArguments.BindBuffer(1, surfelGIIndirectArguments, 0);

                    RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::SurfelGIIndirectAruguments);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        1,
                        1,
                        1);
                };
            });*/
    }

    RenderGraphTextureHandle RealTimeRenderer::AddSurfleGIVisualizationPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(sceneTextures.sceneColorTextureDesc, "SurfelGIVisualizationTexture");

        renderGraph.AddPass(std::format("SurfelGIVisualization (Compute, {}x{})", renderResolution.width, renderResolution.height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto sceneColorTexture = builder.ReadTexture(sceneTextures.sceneColorTexture, RenderBackendResourceState::ShaderResource);
                auto sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 groupCountX = ComputeWorkGroupCount(targetResolution.width, PostProcessingThreadGroupCountX);
                    uint32 groupCountY = ComputeWorkGroupCount(targetResolution.height, PostProcessingThreadGroupCountY);

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneColorTexture)));
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(sceneDepthTexture)));
                    shaderArguments.BindBuffer(3, surfelGIInfoBuffer, 0);
                    shaderArguments.BindBuffer(4, surfelGICellHeaderBuffer, 0);
                    shaderArguments.BindBuffer(5, surfelGICellDataBuffer, 0);
                    shaderArguments.BindBuffer(6, surfelGISurfelHotDataBuffer, 0);
                    shaderArguments.BindTextureUAV(7, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(outputTexture), 0));

                    RenderBackendShaderProgramHandle computeShader = shaderLibrary->GetShaderProgramHandle(ShaderID::SurfelGIVisualization);
                    commandList.Dispatch2D(
                        computeShader,
                        shaderArguments,
                        groupCountX,
                        groupCountY);
                };
            });

        sceneTextures.sceneColorTexture = outputTexture;

        return outputTexture;
    }
}