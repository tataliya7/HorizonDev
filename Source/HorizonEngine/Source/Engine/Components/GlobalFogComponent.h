#pragma once

#include "Foundation/FoundationModule.h"
#include "Engine/Serialization/Archive.h"

namespace Horizon
{
    class RenderScene;
    class GlobalFogRenderObject;

    class GlobalFogComponent
    {
    public:

        float distance = 100.0f;

        Vector3f scattering = Vector3f(0.0f, 0.0f, 0.0f);
        Vector3f absorption = Vector3f(0.0f, 0.0f, 0.0f);
        Vector3f emission = Vector3f(0.0f, 0.0f, 0.0f);
        float phaseG = 0.2f;

        GlobalFogComponent();
        ~GlobalFogComponent();

        bool IsRenderObjectValid() const;
        void CreateRenderObject(RenderScene* scene);
        void DestroyRenderObject(RenderScene* scene);
        void UpdateRenderObject();

    private:

        GlobalFogRenderObject* renderObject = nullptr;
    };
}