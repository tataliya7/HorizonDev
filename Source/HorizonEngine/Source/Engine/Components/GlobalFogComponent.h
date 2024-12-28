#pragma once

#include "Foundation/FoundationModule.h"
#include "Engine/Serialization/Archive.h"

namespace Horizon
{
    class GlobalFogComponent
    {
    public:

        Vector3f scattering;
        Vector3f absorption;
        Vector3f emission;

    private:

        //GlobalFogRenderObject* renderObject = nullptr;
    };
}