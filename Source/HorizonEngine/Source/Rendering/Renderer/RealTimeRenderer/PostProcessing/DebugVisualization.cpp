#include "DebugVisualization.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::AddVisualizePrimitiveIDPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        const auto& finalTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();

        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(finalTextureData.finalTextureDesc, "VisualizePrimitiveIDTexture");

        renderGraph.AddPass(std::format("VisualizePrimitiveID (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeWorkGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeWorkGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(vbuffer0)));
                    shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(outputTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizePrimitiveID);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeMaterialIDPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        const auto& finalTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();

        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(finalTextureData.finalTextureDesc, "VisualizeMaterialIDTexture");

        renderGraph.AddPass(std::format("VisualizeMaterialID (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto vbuffer0 = builder.ReadTexture(sceneTextures.vbuffer0, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeWorkGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeWorkGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindBuffer(1, renderEngine->geometryBuffer);
                    shaderArguments.BindTextureSRV(2, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(vbuffer0)));
                    shaderArguments.BindTextureUAV(3, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(outputTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizeMaterialID);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeMotionVectorsPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        const auto& finalTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();

        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(finalTextureData.finalTextureDesc, "VisualizeMotionVectorsTexture");

        renderGraph.AddPass(std::format("VisualizeMotionVectors (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto motionVectorTexture = builder.ReadTexture(sceneTextures.motionVectorTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeWorkGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeWorkGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(motionVectorTexture)));
                    shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(outputTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizeMotionVectors);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeAmbientOcclusionPass(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        const auto& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        const auto& finalTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();

        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(finalTextureData.finalTextureDesc, "VisualizeAmbientOcclusionTexture");

        renderGraph.AddPass(std::format("VisualizeAmbientOcclusion (Compute, {}x{}->{}x{})", renderResolution.width, renderResolution.height, targetResolution.width, targetResolution.height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto ambientOcclusionTexture = builder.ReadTexture(sceneTextures.ambientOcclusionTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeWorkGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeWorkGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(ambientOcclusionTexture)));
                    shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(outputTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizeAmbientOcclusion);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }

    RenderGraphTextureHandle RealTimeRenderer::AddVisualizeShadowMaskPass(
        RenderGraph& renderGraph,
        const SceneView& view,
        RenderGraphTextureHandle sceneColorTexture)
    {
        auto& debugViewModeTextures = renderGraph.blackboard.Get<RealTimeRendererDebugViewModeTextures>();
        const auto& finalTextureData = renderGraph.blackboard.Get<RenderGraphFinalTexture>();

        if (!debugViewModeTextures.screenSpaceShadowMaskTexture)
        {
            return sceneColorTexture;
        }

        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(finalTextureData.finalTextureDesc, "VisualizeScreenSpaceShadowMaskTexture");

        uint32 width = debugViewModeTextures.screenSpaceShadowMaskTextureDesc.width;
        uint32 height = debugViewModeTextures.screenSpaceShadowMaskTextureDesc.height;

        renderGraph.AddPass(std::format("VisualizeScreenSpaceShadowMask (Compute, {}x{})", width, height, targetResolution.width, targetResolution.height), RenderGraphPassFlags::Compute,
            [&](RenderGraphBuilder& builder)
            {
                auto screenSpaceShadowMaskTexture = builder.ReadTexture(debugViewModeTextures.screenSpaceShadowMaskTexture, RenderBackendResourceState::ShaderResource);

                outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    uint32 threadGroupCountX = ComputeWorkGroupCount(targetResolution.width, PostProcessingThreadGroupSizeX);
                    uint32 threadGroupCountY = ComputeWorkGroupCount(targetResolution.height, PostProcessingThreadGroupSizeY);
                    uint32 threadGroupCountZ = 1;

                    RenderBackendShaderArguments shaderArguments = {};
                    shaderArguments.BindBuffer(0, this->GetCurrentPerFrameDataBuffer());
                    shaderArguments.BindTextureSRV(1, RenderBackendTextureSRVDesc::Create(registry.GetRenderBackendTextureHandle(screenSpaceShadowMaskTexture)));
                    shaderArguments.BindTextureUAV(2, RenderBackendTextureUAVDesc::Create(registry.GetRenderBackendTextureHandle(outputTexture), 0));

                    RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::VisualizeScreenSpaceShadowMask);
                    commandList.Dispatch(
                        computeShader,
                        shaderArguments,
                        threadGroupCountX,
                        threadGroupCountY,
                        threadGroupCountZ);
                };
            });

        return outputTexture;
    }
}