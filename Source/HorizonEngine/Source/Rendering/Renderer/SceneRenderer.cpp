#include "SceneRenderer.h"
#include "SceneView.h"
#include "RealTimeRenderer/RealTimeRendererModule.h"

namespace Horizon
{
    /*SceneRenderer::SceneRenderer(SceneView* sceneView)
        : sceneView(sceneView)
    {

    }

    SceneRenderer::~SceneRenderer()
    {

    }

    SceneRenderer* CreateSceneRenderer(SceneView* sceneView)
    {
        assert(sceneView);

        SceneRenderer* sceneRenderer = nullptr;

        RendererType rendererType = sceneView->GetRenderSettings().rendererType;

        if (rendererType == RendererType::ReferencePathTracing)
        {
            assert(false && "Path tracing renderer is not implemented yet.");
            sceneRenderer = nullptr;
        }
        else if (rendererType == RendererType::RealTime)
        {
            sceneRenderer = new RealTimeRenderer(sceneView);
        }

        return sceneRenderer;
    }

    void DestroySceneRenderer(SceneRenderer* sceneRenderer)
    {
        assert(sceneRenderer != nullptr);

        RenderBackend* renderBackend = sceneRenderer->GetRenderBackend();

        renderBackend->FlushRenderDevices();

        delete sceneRenderer;
    }*/
}