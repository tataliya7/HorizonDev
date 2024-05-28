#pragma once

#include "Engine/HorizonEngineModule.h"

namespace Horizon
{
    class EditorSceneManager
    {
    public:
        Scene* GetActiveScene() const;
    };
}