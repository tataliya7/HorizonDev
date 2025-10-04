#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    class SceneView;
    class RenderBackend;
    class ShaderRepository;
    class RenderGraphResourcePool;

    class SceneRenderer
    {
    public:

        virtual ~SceneRenderer() = default;

        virtual void Tick(float deltaTimeInSeconds) = 0;

        virtual void InitializeSceneView(SceneView* sceneView) = 0;

        virtual void Render(RenderGraph& renderGraph) = 0;
    };

    extern SceneRenderer* CreateSceneRenderer(SceneView* sceneView);

    extern void DestroySceneRenderer(SceneRenderer* sceneRenderer);
}