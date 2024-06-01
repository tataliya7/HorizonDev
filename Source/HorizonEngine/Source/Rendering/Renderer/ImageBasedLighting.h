#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    class ShaderLibrary;

    extern uint32 GPreIntegratedBrdfLutSize;

    extern uint32 GIrradianceEnvironmentMapSize;

    extern void RenderPreIntegratedBrdfLut(ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle preIntegratedBrdfLut);
}