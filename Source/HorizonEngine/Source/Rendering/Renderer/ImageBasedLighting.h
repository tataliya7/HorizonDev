#pragma once

#include "RendererCommon.h"

namespace Horizon
{
    class RenderBackend;
    class ShaderRepository;
    class RenderBackendCommandList;

    // @todo Move this function to other place.
    extern void ConvertLatLongToCubemap(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList* commandList,
        RenderBackendTextureHandle latLongTexture,
        RenderBackendTextureHandle cubemapTexture,
        uint32 cubemapTextureSize);

    // @todo Move this function to other place.
    extern void GenerateCubemapMips(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList* commandList,
        RenderBackendTextureHandle cubemapTexture,
        uint32 mipLevelCount);

    extern uint32 EnvironmentBrdfLutTextureSize;

    extern void PrecomputeEnvironmentBRDFLookupTable(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList* commandList,
        RenderBackendTextureHandle environmentBrdfLutTexture);

    extern void PrecomputeEnvironmentMaps(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList* commandList,
        uint32 environmentMapTextureSize,
        RenderBackendTextureHandle environmentMapTexture,
        uint32 irradianceEnvironmentMapTextureSize,
        RenderBackendTextureHandle convolvedEnvironmentMapTexture,
        RenderBackendTextureHandle irradianceEnvironmentMapTexture);

    extern void PrecomputeEnvironmentMaps(
        RenderBackend* renderBackend,
        ShaderRepository* shaderRepository,
        RenderBackendCommandList* commandList,
        uint32 environmentMapTextureSize,
        RenderBackendTextureHandle environmentMapTexture,
        RenderBackendTextureHandle convolvedEnvironmentMapTexture,
        RenderBackendBufferHandle irradianceEnvironmentMapBuffer);
}