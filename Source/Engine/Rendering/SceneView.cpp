#include "SceneView.h"
#include "Rendering/Renderer/Renderer.h"

namespace HE
{
    RenderBackendRayTracingAccelerationStructureHandle SceneView::GetRayTracingScene() const
    {
        return ((Renderer*)renderEngine)->rayTracingScene;
    }
}