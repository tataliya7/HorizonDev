#include "GlobalFogComponent.h"
#include "Rendering/RenderingModule.h"

namespace Horizon
{
    GlobalFogComponent::GlobalFogComponent()
    {
        renderObject = nullptr;
    }

    GlobalFogComponent::~GlobalFogComponent()
    {
        assert(renderObject == nullptr);
    }

    bool GlobalFogComponent::IsRenderObjectValid() const
    {
        return renderObject != nullptr;
    }

    void GlobalFogComponent::CreateRenderObject(RenderScene* scene)
    {
        assert(renderObject == nullptr);
        if (renderObject == nullptr)
        {
            renderObject = new GlobalFogRenderObject();
            renderObject->distance = distance;
            renderObject->absorption = absorption;
            renderObject->scattering = scattering;
            renderObject->emission = emission;
            renderObject->phaseG = phaseG;
            scene->AddGlobalFog(renderObject);
        }
    }

    void GlobalFogComponent::DestroyRenderObject(RenderScene* scene)
    {

    }

    void GlobalFogComponent::UpdateRenderObject()
    {
        if (renderObject)
        {
            renderObject->distance = distance;
            renderObject->absorption = absorption;
            renderObject->scattering = scattering;
            renderObject->emission = emission;
            renderObject->phaseG = phaseG;
        }
    }
}