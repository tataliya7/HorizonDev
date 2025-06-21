#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

namespace Horizon
{
    RenderBackendTextureHandle LoadTextureFromHDRFile(RenderBackend* renderBackend, const char* filename, RenderBackendTextureDesc* outDesc = nullptr);

    RenderBackendTextureHandle LoadTextureFromFile(RenderBackend* renderBackend, ShaderCollection* shaderLibrary, const char* filename, bool autoMipmaps = true, bool flipY = true, RenderBackendTextureFormat format = RenderBackendTextureFormat::B8G8R8A8Unorm);
}