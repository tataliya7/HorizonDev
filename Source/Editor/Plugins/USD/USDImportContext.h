#pragma once

#include "USD.h"

namespace HE::USDImporter
{
    struct USDImportContext
    {
        USDImportSettings* importSettings;
        std::vector<std::string> materialPaths;
        std::map<std::string, Material> materialMap;
        std::map<std::string, RenderBackendTextureHandle> textureMap;
    };
}