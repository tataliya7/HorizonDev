#pragma once

#include "Rendering/Renderer/RendererCommon.h"
#include "Rendering/Renderer/RendererInterface.h"

namespace Horizon
{
    class PathTracingRenderer : public SceneRenderer
    {
    public:

        PathTracingRenderer(RenderBackend* backend, ShaderCompiler* compiler, RenderSystem* renderEngine);
        virtual ~PathTracingRenderer();

        bool LoadShaders();

        void SetupRenderGraph(RenderGraph& renderGraph, const SceneView& view) override;
    private:

        RenderBackend* renderBackend;
        ShaderCompiler* shaderCompiler;
        ShaderLibrary_DEPRECATED* shaderLibrary;
        RenderSystem* renderEngine;

        PathTracingRendererSceneViewShaderParameters sceneViewShaderParameters;

        RenderBackendRayTracingPipelineStateHandle pathTracingPipelineState;
        RenderBackendBufferHandle pathTracingSBT;
    };
}
