#pragma once

#include "Core/CoreModule.h"

namespace Horizon
{
    class PhysicsScene
    {
    public:

        /**
         * Release this scene.
         */
        virtual void Release() = 0;
    };
}