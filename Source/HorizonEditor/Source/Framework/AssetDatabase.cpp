#include "AssetDatabase.h"

#include "USDModule.h"

namespace Horizon
{
    bool IsUniversalSceneDescriptionFile(const std::filesystem::path& path)
    {
        const std::string& extension = path.extension().string();
        return path.has_extension() && ((extension == ".usd") || (extension == ".usdc") || (extension == ".usdz") || (extension == ".usda"));
    }

    void AssetDatabase::CreateAsset(const std::string& path)
    {
    }

    void AssetDatabase::ImportAsset(const std::filesystem::path& path, Scene* scene)
    {
        if (IsUniversalSceneDescriptionFile(path))
        {
            USDImportSettings settings = {};
            settings.importCameras = true;
            settings.importLights = true;
            settings.importMeshes = true;
            settings.importMaterials = true;
            settings.importSkeletons = true;
            USDImport(scene, path.string().c_str(), &settings, false);
        }
    }

    void AssetDatabase::OpenAsset()
    {

    }
}