#pragma once

#include "Foundation/FoundationModule.h"

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