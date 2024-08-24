#pragma once

#include "Foundation/FoundationModule.h"
#include "Engine/Serialization/SerializationModule.h"

namespace Horizon
{
    class RenderScene;
    class LocalVolumetricFogRenderObject;

    class LocalVolumetricFogComponent
    {
    public:

        LocalVolumetricFogComponent();

        ~LocalVolumetricFogComponent();

        bool IsRenderObjectValid() const;

        void CreateRenderObject(RenderScene* scene);

        void DestroyRenderObject(RenderScene* scene);

        void UpdateRenderObject(const Matrix4x4f& transform);

        Vector3 emission;

    private:

        LocalVolumetricFogRenderObject* renderObject = nullptr;
    };
}