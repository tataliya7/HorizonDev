#pragma once

#include "Engine/HorizonEngineModule.h"

namespace Horizon
{
    RenderBackendTextureHandle LoadTextureFromHDRFile(RenderBackend* renderBackend, const char* filename, RenderBackendTextureDesc* outDesc = nullptr);

    RenderBackendTextureHandle LoadTextureFromFile(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, const char* filename, bool autoMipmaps = true, bool flipY = true, RenderBackendTextureFormat format = RenderBackendTextureFormat::BGRA8Unorm);
}