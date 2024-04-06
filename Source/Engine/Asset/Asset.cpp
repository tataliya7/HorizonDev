#include "Asset.h"

namespace HE
{
    std::unordered_map<std::string, std::shared_ptr<Asset>> AssetManager::ImportedAssets;
}