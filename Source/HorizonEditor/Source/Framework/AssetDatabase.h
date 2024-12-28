#pragma once

#include "Engine/HorizonEngineModule.h"

namespace Horizon
{
    class AssetDatabase
    {
    public:

        void CreateAsset(const std::string& path);

        void ImportAsset(const std::filesystem::path& path, Scene* scene);

        /** Open the specified asset with the corresponding tool. */
        void OpenAsset();

        //void Refresh();
    };
}