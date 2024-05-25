#include "RealTimeRenderer.h"

namespace Horizon
{
    RenderGraphTextureHandle RealTimeRenderer::RenderLocalLightShadows(
        RenderGraph& renderGraph,
        const SceneView& view)
    {
        static const uint32 cubeShadowMapSize = 256;
        static const uint32 localLightShadowMapAtlasSize = 4096;
        RenderGraphTextureDesc localLightShadowMapAtlasDesc = RenderGraphTextureDesc::Create2DArray(
            localLightShadowMapAtlasSize,
            localLightShadowMapAtlasSize,
            RenderBackendTextureFormat::D32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::DepthStencil,
            2,
            RenderBackendTextureClearValue::CreateDepthValue(FarClipPlaneDepthValue));
        auto localLightShadowMapAtlas = renderGraph.CreateTexture(localLightShadowMapAtlasDesc, "LocalLightShadowMapAtlas");

        uint32 numViewports = renderEngine->numCubeShadowMaps * 6;

        renderGraph.AddPass(std::format("LocalLightShadows (Graphics, {}x{}, {} viewports)", localLightShadowMapAtlasSize, localLightShadowMapAtlasSize, numViewports), RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                localLightShadowMapAtlas = builder.WriteTexture(localLightShadowMapAtlas, RenderBackendResourceState::DepthStencil);

                builder.BindDepthTarget(localLightShadowMapAtlas, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewports[RenderBackendMaxViewportCount];
                    RenderBackendScissor scissors[RenderBackendMaxViewportCount];

                    uint32 viewportIndex = 0;
                    for (uint32 cubeShadowMapIndex = 0; cubeShadowMapIndex < renderEngine->numCubeShadowMaps; cubeShadowMapIndex++)
                    {
                        for (uint32 faceIndex = 0; faceIndex < 6; faceIndex++)
                        {
                            viewports[viewportIndex] = RenderBackendViewport((float)faceIndex * cubeShadowMapSize, (float)cubeShadowMapIndex * cubeShadowMapSize, (float)cubeShadowMapSize, (float)cubeShadowMapSize);
                            scissors[viewportIndex] = RenderBackendScissor(0, 0, localLightShadowMapAtlasSize, localLightShadowMapAtlasSize);
                            viewportIndex++;
                        }
                    }

                    commandList.SetViewports(viewports, numViewports);
                    commandList.SetScissors(scissors, numViewports);

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::Back;
                    graphicsPipelineState.rasterizationState.depthBiasSlopeFactor = -1.5f;
                    graphicsPipelineState.rasterizationState.depthBiasConstantFactor = -1.25f;
                    graphicsPipelineState.depthStencilState.depthTestEnable = true;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = true;
                    graphicsPipelineState.depthStencilState.depthCompareFunction = RenderBackendCompareOp::GreaterOrEqual;

                    for (const auto& drawCallInfo : renderEngine->drawList)
                    {
                        RenderBackendShaderArguments shaderArguments = {};
                        shaderArguments.BindBuffer(0, GetCurrentPerFrameDataBuffer());
                        shaderArguments.BindBuffer(1, renderEngine->geometryBuffer, drawCallInfo.geometryIndex * sizeof(GeometryShaderParameters));
                        shaderArguments.BindBuffer(2, renderEngine->materialBuffer, 0);
                        shaderArguments.BindBuffer(3, renderEngine->cubeShadowMapBuffer, sizeof(CubeShadowMapShaderParameters));

                        RenderBackendShaderProgramHandle graphicsShader = shaderLibrary->GetShaderProgramHandle(ShaderID::LocalLightShadows);

                        commandList.DrawIndexed(
                            graphicsShader,
                            graphicsPipelineState,
                            shaderArguments,
                            drawCallInfo.indexBuffer,
                            drawCallInfo.numIndices,
                            numViewports,
                            drawCallInfo.firstIndex,
                            0,
                            0,
                            RenderBackendPrimitiveTopology::TriangleList);
                    }

                    // Restore viewport and scissor
                    RenderBackendViewport viewport(0.0f, 0.0f, (float)renderResolution.width, (float)renderResolution.height);
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, renderResolution.width, renderResolution.height);
                    commandList.SetScissors(&scissor, 1);
                };
            });

        return localLightShadowMapAtlas;
    }
}

