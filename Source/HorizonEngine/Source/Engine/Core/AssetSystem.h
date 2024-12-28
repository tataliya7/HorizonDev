#pragma once

#include "Engine/Core/Subsystem.h"
#include "Engine/AssetManagement/RuntimeAssetDatabase.h"

namespace Horizon
{
    class AssetSystem final : public Subsystem
    {
    public:

        AssetSystem();
        ~AssetSystem();

        void Init() override;

        void Exit() override;

        void Tick(float deltaTimeInSeconds);

        RuntimeAssetDatabase* GetAssetDatabase()
        {
            return assetDatabase;
        }

    private:

        RuntimeAssetDatabase* assetDatabase = nullptr;
    };
}