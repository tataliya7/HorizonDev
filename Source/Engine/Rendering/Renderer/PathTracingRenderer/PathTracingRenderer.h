#pragma once

#include "Core/CoreModule.h"
#include "Entity/EntityModule.h"
#include "RenderBackend/RenderBackendModule.h"
#include "Rendering/RenderGraph/RenderGraph.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/ShaderLibrary.h"
#include "Rendering/SceneView.h"
#include "Rendering/RenderSystem.h"

namespace HE
{
    struct PathTracingRendererSceneViewShaderParameters
    {

    };

    class PathTracingRenderer : public RenderPipeline
    {
    public:

        PathTracingRenderer(RenderBackend* backend, ShaderCompiler* compiler, RenderSystem* renderEngine);
        virtual ~PathTracingRenderer();

        bool LoadShaders();

        void SetupRenderGraph(RenderGraph& renderGraph, const SceneView& view) override;
    private:

        RenderBackend* renderBackend;
        ShaderCompiler* shaderCompiler;
        ShaderLibrary_Deprecated* shaderLibrary;
        RenderSystem* renderEngine;

        PathTracingRendererSceneViewShaderParameters sceneViewShaderParameters;

        RenderBackendRayTracingPipelineStateHandle pathTracingPipelineState;
        RenderBackendBufferHandle pathTracingSBT;
    };
}
