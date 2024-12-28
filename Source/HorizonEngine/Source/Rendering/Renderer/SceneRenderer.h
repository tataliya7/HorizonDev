#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    static constexpr float NearClippingPlaneDepthValue = 1.0f;
    static constexpr float FarClippingPlaneDepthValue = 0.0f;

    enum class RenderMode
    {
        Rasterization,
        HybridRendering,
        RealTimePathTracing,
        ReferencePathTracing,
        Count,
    };

    //class SceneView;

    ///**
    // * The renderer implements the process of generating visual images.
    // */
    //class SceneRenderer
    //{
    //public:
    //    SceneRenderer();
    //    virtual ~SceneRenderer();

    //    virtual void Render(RenderGraph& renderGraph) = 0;

    //    //RenderBackend* GetRenderBackend() const;

    //private:
    //    //SceneView* sceneView;
    //};

    //SceneRenderer* CreateSceneRenderer();
    //void DestroySceneRenderer(SceneRenderer* sceneRenderer);
}