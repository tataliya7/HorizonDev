#pragma once

#include "Core/CoreModule.h"

namespace Horizon
{
    class PhysicsSceneInterface
    {
    public:

        /**
         * Release this scene.
         */
        virtual void Release() = 0;
    };
}