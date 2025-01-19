#pragma once

#include "Engine/HorizonEngineModule.h"

namespace Horizon
{
    struct RenderDocPluginSettings
    {
        std::string installationFolder;
    };

    HORIZON_API void RenderDocPluginInit();

    HORIZON_API void RenderDocPluginTriggerCapture();
}