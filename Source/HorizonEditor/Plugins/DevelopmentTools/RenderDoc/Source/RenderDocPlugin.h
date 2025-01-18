#pragma once

#include "Engine/HorizonEngineModule.h"

namespace Horizon
{
    struct RenderDocPluginSettings
    {
        std::string installationFolder;
    };

    void RenderDocPluginInit();

    void RenderDocPluginTriggerCapture();
}