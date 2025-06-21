#include "SceneRenderer.h"
#include "SceneView.h"
#include "RasterizationRenderer/RasterizationRendererModule.h"

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

        RenderMode renderMode = sceneView->GetRenderSettings().renderMode;

        if (renderMode == RenderMode::ReferencePathTracing)
        {
            assert(false && "Path tracing renderer is not implemented yet.");
            sceneRenderer = nullptr;
        }
        else if (renderMode == RenderMode::Rasterization)
        {
            sceneRenderer = new RasterizationRenderer(sceneView);
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