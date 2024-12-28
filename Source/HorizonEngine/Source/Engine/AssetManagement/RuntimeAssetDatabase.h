#pragma once

#include "Foundation/FoundationModule.h"

namespace Horizon
{
    class RuntimeAssetDatabase
    {
    public:

        void CreateAsset(const std::string& path);

        void ImportAsset(const std::string& path);

        /** Open the specified asset with the corresponding tool. */
        void OpenAsset();

        //void Refresh();
    };
}