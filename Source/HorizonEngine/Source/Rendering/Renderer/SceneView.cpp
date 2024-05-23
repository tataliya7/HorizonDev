#include "SceneView.h"
#include "Rendering/RenderSystem.h"

namespace Horizon
{
    SceneView::SceneView(RenderScene* scene)
        : scene(scene)
        , cameraJitterOffset(ZeroVector2)
    {

    }
}