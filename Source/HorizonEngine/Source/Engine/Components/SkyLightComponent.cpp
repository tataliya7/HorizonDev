#include "SkyLightComponent.h"
#include "Rendering/RenderingModule.h"

namespace Horizon
{
    void SkyLightComponent::CreateRenderObject(RenderScene* scene)
    {
        assert(renderObject == nullptr);
        if (renderObject == nullptr)
        {
            renderObject = new SkyLightRenderObject();
            renderObject->cubemapSize = cubemapSize;
            renderObject->environmentMapTexture = environmentMapTexture;
            scene->AddSkyLight(renderObject);
        }
    }

    void SkyLightComponent::DestroyRenderObject(RenderScene* scene)
    {

    }

    void SkyLightComponent::UpdateRenderObject()
    {

    }
}