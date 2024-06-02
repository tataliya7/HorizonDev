#include "LightComponent.h"
#include "Rendering/RenderingModule.h"

namespace Horizon
{
    void LightComponent::CreateRenderObject(RenderScene* scene)
    {
        if (true)
        {
            LightRenderObjectDescription description;
            description.color = color * luminousIntensity;
            description.position = Vector3(0.0f, 0.0f, 0.0f);
            description.direction = forwardVec;
            description.castRayTracingShadows = castShadows;
            description.usedAsAtmosphericLight = usedAsAtmosphericLight;

            renderObject = new LightRenderObject(description);
            scene->AddLight(renderObject);
        }
    }

    void LightComponent::DestroyRenderObject(RenderScene* scene)
    {

    }

    void LightComponent::UpdateRenderObject()
    {
        if (renderObject)
        {

        }
    }
}