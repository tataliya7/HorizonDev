#pragma once

#include "USD.h"

namespace Horizon::USDImporter
{
    struct USDImportContext
    {
        USDImportSettings* importSettings;
        std::vector<std::string> materialPaths;
        std::vector<std::string> skeletonPaths;
        std::vector<std::string> animationPaths;
        std::map<std::string, Material> materialMap;
        std::map<std::string, RenderBackendTextureHandle> textureMap;
        std::map<std::string, Skeleton*> skeletons;
        std::map<std::string, SkeletonAnimation*> animations;
    };
}