#pragma once

#include "RenderBackend/RenderBackendModule.h"

namespace HE
{
    struct Color
    {
        float r;
        float g;
        float b;
        float a;
    };

    class SceneView;
    
    /**
     * Everything starts here.
     */
    void RenderSceneView(SceneView* view);

    class TextureImportSettings
    {

    };

    RenderBackendTextureHandle LoadTextureFromHDRFile(RenderBackend* renderBackend, const char* filename, RenderBackendTextureDesc* outDesc = nullptr);
    RenderBackendTextureHandle LoadTextureFromFile(RenderBackend* renderBackend, const char* filename, bool autoMipmaps = true, bool filpY = true, RenderBackendTextureFormat format = RenderBackendTextureFormat::BGRA8Unorm);
}