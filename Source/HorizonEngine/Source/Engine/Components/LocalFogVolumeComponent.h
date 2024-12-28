#pragma once

#include "Foundation/FoundationModule.h"
#include "Engine/Serialization/SerializationModule.h"

namespace Horizon
{
    class RenderScene;
    class LocalFogVolumeRenderObject;

    class LocalFogVolumeComponent
    {
    public:

        LocalFogVolumeComponent();

        ~LocalFogVolumeComponent();

        bool IsRenderObjectValid() const;

        void CreateRenderObject(RenderScene* scene);

        void DestroyRenderObject(RenderScene* scene);

        void UpdateRenderObject(const Matrix4x4f& transform);

        Vector3f scattering;
        Vector3f absorption;
        Vector3f emission;

    private:

        LocalFogVolumeRenderObject* renderObject = nullptr;
    };
}