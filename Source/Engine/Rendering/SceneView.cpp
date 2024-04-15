#include "SceneView.h"
#include "Rendering/RenderSystem.h"

namespace HE
{
    RenderBackendRayTracingAccelerationStructureHandle SceneView::GetRayTracingScene() const
    {
        return ((RenderSystem*)renderEngine)->rayTracingScene;
    }
}