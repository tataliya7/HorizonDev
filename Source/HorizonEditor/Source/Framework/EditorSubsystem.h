#pragma once

#include "Engine/HorizonEngineModule.h"

namespace Horizon
{
    class EditorSystem : public Subsystem
    {

    };
}

#define HE_BIND_FUNCTION(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args) > (args)...); }
