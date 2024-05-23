#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    class ShaderLibrary_DEPRECATED;

    extern uint32 GPreIntegratedBrdfLutSize;

    extern uint32 GIrradianceEnvironmentMapSize;

    extern void RenderPreIntegratedBrdfLut(ShaderLibrary_DEPRECATED* shaderLibrary, RenderBackendCommandList& commandList);
}