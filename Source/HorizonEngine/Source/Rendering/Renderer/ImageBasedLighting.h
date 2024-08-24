#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    class ShaderLibrary;

    extern uint32 GPreIntegratedBrdfLutSize;

    extern uint32 GIrradianceEnvironmentMapSize;

    extern void RenderPreIntegratedBrdfLut(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle preIntegratedBrdfLut);

    // TODO: move to other place
    extern void ConvertLatLongToCubemap(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle latLongTexture, RenderBackendTextureHandle cubemapTexture, uint32 cubemapTextureSize);
    extern void GenerateCubemapMips(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle cubemap, uint32 numMipLevels);
}