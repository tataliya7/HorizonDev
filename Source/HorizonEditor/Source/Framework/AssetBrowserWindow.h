#pragma once

#include "Engine/HorizonEngineModule.h"

#include "HorizonEditor.h"

#include <mutex>
#include <filesystem>

#include <imgui.h>
#include <imgui_internal.h>

namespace Horizon
{
    namespace UI
    {
        inline bool ImageButton(const char* stringID, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1))
        {
            return ImGui::ImageButtonEx(ImGui::GetID(stringID), textureID, size, uv0, uv1, bg_col, tint_col);
        }
    }

    enum class AssetBrowserActionFlags
    {
        None = 0,
        Refresh = 1 << 0,
        ClearSelections = 1 << 1,
        Selected = 1 << 2,
        Deselected = 1 << 3,
        Hovered = 1 << 4,
        Renamed = 1 << 5,
        ChangeDirectory = 1 << 6,
        DeleteSelectedItems = 1 << 7,
        SelectToHere = 1 << 8,
        Moved = 1 << 9,
        ShowInExplorer = 1 << 10,
        OpenExternally = 1 << 11,
        Reload = 1 << 12,
    };
    HORIZON_OVERLOAD_ENUM_CLASS_OPERATORS(AssetBrowserActionFlags);

    enum class AssetBrowserItemType
    {
        Directory,
        File,
    };

    #define MAX_INPUT_BUFFER_LENGTH 128
    class AssetBrowserItem
    {
    public:
        AssetBrowserItem(const std::string& name, const std::filesystem::path& path, AssetBrowserItemType type, RenderBackendTextureHandle icon)
            : name(name), path(path), type(type), icon(icon) {}
        virtual ~AssetBrowserItem() {}
        bool IsSelected() const { return isSelected; }
        AssetBrowserItemType GetType() const { return type; }
        const std::string& GetName() const { return name; }
        RenderBackendTextureHandle GetIcon() const { return icon; }
        std::filesystem::path GetPath() const { return path; }
    private:
        friend class AssetBrowserWindow;
        virtual void RenderCustomContextItems() {}
    protected:
        std::string name;
        std::filesystem::path path;
        AssetBrowserItemType type;
        RenderBackendTextureHandle icon;
        bool isSelected = false;
        bool isRenaming = false;
        bool isDragging = false;
    };

    class AssetBrowserDirectory : public AssetBrowserItem
    {
    public:
        AssetBrowserDirectory(std::filesystem::path path, AssetBrowserDirectory* parent, RenderBackendTextureHandle icon)
            : AssetBrowserItem(path.filename().string(), path, AssetBrowserItemType::Directory, icon), parent(parent) {}
        ~AssetBrowserDirectory() {}
    private:
        friend class AssetBrowserWindow;
        AssetBrowserDirectory* parent;
        std::vector<std::filesystem::path> assets;
        std::unordered_map<std::string, AssetBrowserDirectory*> subdirectories;
    };

    class AssetBrowserAsset : public AssetBrowserItem
    {
    public:
        AssetBrowserAsset(std::filesystem::path path, RenderBackendTextureHandle icon)
            : AssetBrowserItem(path.stem().string(), path, AssetBrowserItemType::File, icon) {}
        ~AssetBrowserAsset() {}
    private:
        friend class AssetBrowserWindow;
    };

    class AssetBrowserSelectionStack
    {
    public:
        void CopyFrom(const AssetBrowserSelectionStack& other)
        {
            items.assign(other.begin(), other.end());
        }
        bool Contains(const std::filesystem::path& guid) const
        {
            for (std::filesystem::path id : items)
            {
                if (id.string() == guid.string())
                {
                    return true;
                }
            }
            return false;
        }
        void Add(const std::filesystem::path& guid)
        {
            if (Contains(guid))
            {
                return;
            }
            items.push_back(guid);
        }
        void Remove(const std::filesystem::path& guid)
        {
            if (!Contains(guid))
            {
                return;
            }
            for (auto it = items.begin(); it != items.end(); it++)
            {
                if (guid.string() == it->string())
                {
                    items.erase(it);
                    break;
                }
            }
        }
        void Clear()
        {
            items.clear();
        }
        uint32 GetCount() const { return (uint32)items.size(); }
        const std::filesystem::path* GetData() const { return items.data(); }
        const std::filesystem::path& operator[](uint32 index) const { return items[index]; }
        std::vector<std::filesystem::path>::iterator begin() { return items.begin(); }
        std::vector<std::filesystem::path>::iterator end() { return items.end(); }
        std::vector<std::filesystem::path>::const_iterator begin() const { return items.begin(); }
        std::vector<std::filesystem::path>::const_iterator end() const { return items.end(); }
    private:
        std::vector<std::filesystem::path> items;
    };

    struct AssetBrowserItemList
    {
        static const uint32 InvalidIndex = std::numeric_limits<uint32>::max();
        std::vector<AssetBrowserItem> items;
        std::vector<AssetBrowserItem>::iterator begin() { return items.begin(); }
        std::vector<AssetBrowserItem>::iterator end() { return items.end(); }
        std::vector<AssetBrowserItem>::const_iterator begin() const { return items.begin(); }
        std::vector<AssetBrowserItem>::const_iterator end() const { return items.end(); }
        AssetBrowserItem& operator[](uint32 index) { return items[index]; }
        const AssetBrowserItem& operator[](uint32 index) const { return items[index]; }
        void Clear()
        {
            items.clear();
        }
        AssetBrowserItem& Add(AssetBrowserDirectory item)
        {
            return items.emplace_back(item);
        }
        AssetBrowserItem& Add(AssetBrowserAsset item)
        {
            return items.emplace_back(item);
        }
        void Remove(const std::filesystem::path& guid)
        {
            uint32 index = Find(guid);
            if (index == InvalidIndex)
            {
                return;
            }
            items.erase(items.begin() + index);
        }
        uint32 Find(const std::filesystem::path& guid) const
        {
            for (uint32 i = 0; i < (uint32)items.size(); i++)
            {
                if (items[i].GetPath().string() == guid.string())
                {
                    return i;
                }
            }
            return InvalidIndex;
        }
    };

    class AssetBrowserWindow
    {
    public:
        AssetBrowserWindow(HorizonEditor* editor);
        ~AssetBrowserWindow();
        void UpdateDropArea(AssetBrowserDirectory* directory);
        void ChangeDirectory(AssetBrowserDirectory* directory);
        void RenderDirectoryHierarchy(AssetBrowserDirectory* directory);
        void RenderTopBar();
        void RenderItems();
        void RenderBottomBar();
        void OnImGuiRender();
        void Refresh();
        void UpdateInput();
        void RenameSelectedItems();
        void DeleteSelectedItems();
        void StartRenamingItem(AssetBrowserItem* item);
        void SelectItem(AssetBrowserItem* item);
        void DeselectItem(AssetBrowserItem* item);
        void ClearSelections();
        void SortItemList();
        AssetBrowserItemList Search(const std::string& content, AssetBrowserDirectory* directory);
    private:
        AssetBrowserDirectory* GetDirectory(const std::filesystem::path& path) const;
        const std::filesystem::path& ProcessDirectory(const std::filesystem::path& path, AssetBrowserDirectory* parent);
        HorizonEditor* editor;
        std::mutex lockMutex;
        bool isHovered;
        bool isFocused;
        bool isAnyItemHovered;
        AssetBrowserItemList currentItems;
        AssetBrowserSelectionStack selectedItems;
        AssetBrowserSelectionStack copiedAssets;
        AssetBrowserDirectory* baseDirectory;
        AssetBrowserDirectory* previousDirectory;
        AssetBrowserDirectory* currentDirectory;
        AssetBrowserDirectory* nextDirectory;
        bool m_UpdateNavigationPath = false;
        std::vector<AssetBrowserDirectory*> m_BreadCrumbData;
        std::unordered_map<std::string, AssetBrowserDirectory*> directories;

        RenderBackendTextureHandle fileIcon;
        RenderBackendTextureHandle directoryIcon;
        RenderBackendTextureHandle backwardButtonIcon;
        RenderBackendTextureHandle forwardButtonIcon;
        RenderBackendTextureHandle refreshButtonIcon;

        std::map<std::string, RenderBackendTextureHandle> iconMap;
        float thumbnailSize = 156.0f;
        float padding = 2.0f;
        float topBarHeight = 34.0f;
        char renameBuffer[MAX_INPUT_BUFFER_LENGTH];
        char searchBuffer[MAX_INPUT_BUFFER_LENGTH];
    };
}