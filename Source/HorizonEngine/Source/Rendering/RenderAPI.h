#pragma once

#include "RenderBackend/RenderBackendModule.h"

namespace Horizon
{
    class TextureImportSettings
    {

    };

    RenderBackendTextureHandle LoadTextureFromHDRFile(RenderBackend* renderBackend, const char* filename, RenderBackendTextureDesc* outDesc = nullptr);
    RenderBackendTextureHandle LoadTextureFromFile(RenderBackend* renderBackend, const char* filename, bool autoMipmaps = true, bool filpY = true, RenderBackendTextureFormat format = RenderBackendTextureFormat::BGRA8Unorm);
}