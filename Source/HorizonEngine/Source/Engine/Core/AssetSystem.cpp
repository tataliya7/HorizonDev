#include "AssetSystem.h"

namespace Horizon
{
    AssetSystem::AssetSystem()
    {

    }

    AssetSystem::~AssetSystem()
    {

    }

    void AssetSystem::Init()
    {
        assetDatabase = new RuntimeAssetDatabase();
    }

    void AssetSystem::Exit()
    {
        delete assetDatabase;
        assetDatabase = nullptr;
    }

    void AssetSystem::Tick(float deltaTimeInSeconds)
    {

    }

}