#pragma once

#include "Foundation/FoundationModule.h"
#include "Engine/Serialization/Archive.h"

namespace Horizon
{
    class GlobalFogComponent
    {
    public:

        Vector3 scattering;
        Vector3 absorption;
        Vector3 emission;

    private:

        //GlobalFogRenderObject* renderObject = nullptr;
    };
}