#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    class ShaderRepository;

    extern uint32 GEnvironmentBrdfLutTextureSize;

    extern uint32 GIrradianceEnvironmentMapSize;

    extern void RenderEnvironmentBrdfLut(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList& commandList,
        RenderBackendTextureHandle environmentBrdfLutTexture);

    // TODO: move to other place
    extern void ConvertLatLongToCubemap(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList& commandList,
        RenderBackendTextureHandle latLongTexture,
        RenderBackendTextureHandle cubemapTexture,
        uint32 cubemapTextureSize);

    extern void GenerateCubemapMips(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList& commandList,
        RenderBackendTextureHandle cubemapTexture,
        uint32 mipLevelCount);

    extern void PrecomputeEnvironmentMaps(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList& commandList,
        uint32 cubemapSize,
        RenderBackendTextureHandle environmentMapTexture,
        RenderBackendTextureHandle convolvedEnvironmentMap,
        RenderBackendTextureHandle irradianceEnvironmentMapTexture,
        RenderBackendBufferHandle irradianceEnvironmentMapBuffer,
        RenderBackendBufferHandle irradianceEnvironmentMapBufferFast);
}