#pragma once

#include "Foundation/FoundationModule.h"

namespace Horizon
{
    // struct Asset
    // {
    //     std::string filename;
    // };
    //
    // class AssetImporter
    // {
    // public:
    //     virtual void ImportAsset(const char* file, EntityHandle& targetEntity) = 0;
    // };
    //
    // class AssetManager
    // {
    // public:
    //     static bool ImportAsset(const std::filesystem::path& filepath)
    //     {
    //         Asset* asset = new Asset();
    //         asset->filename = filepath.string();
    //         ImportedAssets[asset->filename] = std::shared_ptr<Asset>(asset);
    //         return true;
    //     }
    //     static void AddAsset(const std::string& path, Asset* asset)
    //     {
    //         ImportedAssets.emplace(path, asset);
    //     }
    //     template <typename T>
    //     static T* GetAsset(const std::string& assetHandle)
    //     {
    //         if (ImportedAssets.find(assetHandle) == ImportedAssets.end())
    //         {
    //             return nullptr;
    //         }
    //         return (T*)(ImportedAssets[assetHandle].get());
    //     }
    //
    //     static std::unordered_map<std::string, std::shared_ptr<Asset>> ImportedAssets;
    // };
}
