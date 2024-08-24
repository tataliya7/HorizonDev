#include "LocalVolumetricFogComponent.h"

#include "Rendering/Renderer/RenderScene.h"

namespace Horizon
{
    LocalVolumetricFogComponent::LocalVolumetricFogComponent()
    {

    }

    LocalVolumetricFogComponent::~LocalVolumetricFogComponent()
    {

    }

    bool LocalVolumetricFogComponent::IsRenderObjectValid() const
    {
        return renderObject != nullptr;
    }

    void LocalVolumetricFogComponent::CreateRenderObject(RenderScene* scene)
    {
        assert(renderObject == nullptr);
        if (renderObject == nullptr)
        {
            renderObject = new LocalVolumetricFogRenderObject();
            scene->AddLocalVolumetricFog(renderObject);
        }
    }

    void LocalVolumetricFogComponent::DestroyRenderObject(RenderScene* scene)
    {

    }

    void LocalVolumetricFogComponent::UpdateRenderObject(const Matrix4x4f& transform)
    {
        if (renderObject)
        {
            renderObject->transform = transform;
            renderObject->emission = emission;
        }
    }
}