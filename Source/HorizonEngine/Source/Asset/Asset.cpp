#include "Asset.h"

namespace Horizon
{
    std::unordered_map<std::string, std::shared_ptr<Asset>> AssetManager::ImportedAssets;
}