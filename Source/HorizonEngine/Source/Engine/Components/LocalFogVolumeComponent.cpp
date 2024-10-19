#include "LocalFogVolumeComponent.h"

#include "Rendering/Renderer/RenderScene.h"

namespace Horizon
{
    LocalFogVolumeComponent::LocalFogVolumeComponent()
    {

    }

    LocalFogVolumeComponent::~LocalFogVolumeComponent()
    {

    }

    bool LocalFogVolumeComponent::IsRenderObjectValid() const
    {
        return renderObject != nullptr;
    }

    void LocalFogVolumeComponent::CreateRenderObject(RenderScene* scene)
    {
        assert(renderObject == nullptr);
        if (renderObject == nullptr)
        {
            renderObject = new LocalFogVolumeRenderObject();
            scene->AddLocalFogVolume(renderObject);
        }
    }

    void LocalFogVolumeComponent::DestroyRenderObject(RenderScene* scene)
    {

    }

    void LocalFogVolumeComponent::UpdateRenderObject(const Matrix4x4f& transform)
    {
        if (renderObject)
        {
            renderObject->transform = transform;

            renderObject->scattering = scattering;
            renderObject->absorption = absorption;
            renderObject->emission = emission;
        }
    }
}