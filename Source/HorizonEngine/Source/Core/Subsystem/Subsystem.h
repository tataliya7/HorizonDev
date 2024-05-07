#pragma once

#include "Core/CoreModule.h"

namespace Horizon
{
    class Subsystem
    {
    public:
        virtual bool Init() { return true; };
        virtual void Exit() {};
    };
}