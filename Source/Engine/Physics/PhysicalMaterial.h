#pragma once

#include "Core/CoreModule.h"

namespace HE
{
    class PhysicalMaterial
    {
    public:
        PhysicalMaterial();
        virtual ~PhysicalMaterial();
        float staticFriction;
        float dynamicFriction;
        float restitution;
    };
}