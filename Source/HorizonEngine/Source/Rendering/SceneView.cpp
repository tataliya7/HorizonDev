#include "SceneView.h"
#include "Rendering/RenderSystem.h"

namespace Horizon
{
    RenderBackendRayTracingAccelerationStructureHandle SceneView::GetRayTracingScene() const
    {
        return ((RenderSystem*)renderEngine)->rayTracingScene;
    }
}