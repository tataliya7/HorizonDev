#pragma once

#include "Asset/AssetTypes.h"

namespace HE
{
    class AssetRegistry
    {
    public:

        AssetMetaData& operator[](const AssetHandle handle);
        const AssetMetaData& Get(const AssetHandle handle) const;

        size_t Count() const { return registry.size(); }
        bool Contains(const AssetHandle handle) const;
        size_t Remove(const AssetHandle handle);
        void Clear();

        auto begin() { return registry.begin(); }
        auto end() { return registry.end(); }
        auto begin() const { return registry.cbegin(); }
        auto end() const { return registry.cend(); }

    private:

        std::unordered_map<AssetHandle, AssetMetaData> registry;
    };

    class AssetDatabase
    {

    };
}