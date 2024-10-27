#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"
#include "Engine/Serialization/Archive.h"

namespace Horizon
{
    class RenderScene;
    class SkyLightRenderObject;

    class SkyLightComponent
    {
    public:
        bool IsRenderObjectValid() const;
        void CreateRenderObject(RenderScene* scene);
        void DestroyRenderObject(RenderScene* scene);
        void UpdateRenderObject();

        uint32 cubemapSize = 128;

        RenderGraphPersistentTexture environmentMapTexture;

    private:
        SkyLightRenderObject* renderObject = nullptr;
    };
}