#pragma once

#include "Core/CoreModule.h"

namespace HE
{
    class AssetHandle
    {
    public:
        AssetHandle() : guid() {}
        AssetHandle(Guid guid) : guid(guid) {}
        bool operator==(const AssetHandle& rhs) const
        {
            return guid == rhs.guid;
        }
        bool operator!=(const AssetHandle& rhs) const
        {
            return guid != rhs.guid;
        }
        operator bool() const { return guid != Guid(); }
    private:
        Guid guid;
    };

    enum class AssetType
    {
        None                = 0,
        Scene               = 1,
        Mesh                = 2,
        Material            = 3,
        Texture             = 4,
        Audio               = 5,
        PhysicalMaterial    = 6,
        Font                = 7,
        Script              = 8,
    };

    inline static std::unordered_map<std::string, AssetType> GAssetExtensionMap =
    {
        // Horizon types
        { ".horizon", AssetType::Scene },

        // Meshes
        { ".fbx", AssetType::Mesh },
        { ".gltf", AssetType::Mesh },
        { ".glb", AssetType::Mesh },
        { ".obj", AssetType::Mesh },

        // Textures
        { ".dds", AssetType::Texture },
        { ".tga", AssetType::Texture },
        { ".png", AssetType::Texture },
        { ".jpg", AssetType::Texture },
        { ".jpeg", AssetType::Texture },
        { ".hdr", AssetType::Texture },

        // Audio
        { ".wav", AssetType::Audio },

        // Fonts
        { ".ttf", AssetType::Font },
        { ".ttc", AssetType::Font },
        { ".otf", AssetType::Font },
    };

    inline AssetType AssetTypeFromString(const std::string& assetType)
    {
        return AssetType::None;
    }

    inline const char* AssetTypeToString(AssetType assetType)
    {
        return "None";
    }

    struct AssetMetaData
    {
        std::string author;
        std::string description;
    };
}