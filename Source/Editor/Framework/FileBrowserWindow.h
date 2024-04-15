#pragma once

#include <HorizonEngine.h>

#include "HorizonEditor.h"

#include <mutex>
#include <filesystem>

#include <imgui.h>
#include <imgui_internal.h>

namespace HE
{
    namespace UI
    {
        inline bool ImageButton(const char* stringID, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1))
        {
            return ImGui::ImageButtonEx(ImGui::GetID(stringID), textureID, size, uv0, uv1, bg_col, tint_col);
        }
    }

    enum class FileBrowserActionFlags
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
    HE_ENUM_CLASS_OPERATORS(FileBrowserActionFlags);

    enum class FileBrowserItemType
    {
        Directory,
        File,
    };

    #define MAX_INPUT_BUFFER_LENGTH 128
    class FileBrowserItem
    {
    public:
        FileBrowserItem(const std::string& name, const std::filesystem::path& path, FileBrowserItemType type, RenderBackendTextureHandle icon)
            : name(name), path(path), type(type), icon(icon) {}
        virtual ~FileBrowserItem() {}
        bool IsSelected() const { return isSelected; }
        FileBrowserItemType GetType() const { return type; }
        const std::string& GetName() const { return name; }
        RenderBackendTextureHandle GetIcon() const { return icon; }
        std::filesystem::path GetPath() const { return path; }
    private:
        friend class FileBrowserWindow;
        virtual void RenderCustomContextItems() {}
    protected:
        std::string name;
        std::filesystem::path path;
        FileBrowserItemType type;
        RenderBackendTextureHandle icon;
        bool isSelected = false;
        bool isRenaming = false;
        bool isDragging = false;
    };

    class FileBrowserDirectory : public FileBrowserItem
    {
    public:
        FileBrowserDirectory(std::filesystem::path path, FileBrowserDirectory* parent, RenderBackendTextureHandle icon)
            : FileBrowserItem(path.filename().string(), path, FileBrowserItemType::Directory, icon), parent(parent) {}
        ~FileBrowserDirectory() {}
    private:
        friend class FileBrowserWindow;
        FileBrowserDirectory* parent;
        std::vector<std::filesystem::path> assets;
        std::unordered_map<std::string, FileBrowserDirectory*> subdirectories;
    };

    class FileBrowserAsset : public FileBrowserItem
    {
    public:
        FileBrowserAsset(std::filesystem::path path, RenderBackendTextureHandle icon)
            : FileBrowserItem(path.stem().string(), path, FileBrowserItemType::File, icon) {}
        ~FileBrowserAsset() {}
    private:
        friend class FileBrowserWindow;
    };

    class FileBrowserSelectionStack
    {
    public:
        void CopyFrom(const FileBrowserSelectionStack& other)
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

    struct FileBrowserItemList
    {
        static const uint32 InvalidIndex = std::numeric_limits<uint32>::max();
        std::vector<FileBrowserItem> items;
        std::vector<FileBrowserItem>::iterator begin() { return items.begin(); }
        std::vector<FileBrowserItem>::iterator end() { return items.end(); }
        std::vector<FileBrowserItem>::const_iterator begin() const { return items.begin(); }
        std::vector<FileBrowserItem>::const_iterator end() const { return items.end(); }
        FileBrowserItem& operator[](uint32 index) { return items[index]; }
        const FileBrowserItem& operator[](uint32 index) const { return items[index]; }
        void Clear()
        {
            items.clear();
        }
        FileBrowserItem& Add(FileBrowserDirectory item)
        {
            return items.emplace_back(item);
        }
        FileBrowserItem& Add(FileBrowserAsset item)
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

    class FileBrowserWindow
    {
    public:
        FileBrowserWindow(HorizonEditor* editor, AssetManager* assetManager);
        ~FileBrowserWindow();
        void UpdateDropArea(FileBrowserDirectory* directory);
        void ChangeDirectory(FileBrowserDirectory* directory);
        void RenderDirectoryHierarchy(FileBrowserDirectory* directory);
        void RenderTopBar();
        void RenderItems();
        void RenderBottomBar();
        void OnImGuiRender();
        void Refresh();
        void UpdateInput();
        void RenameSelectedItems();
        void DeleteSelectedItems();
        void StartRenamingItem(FileBrowserItem* item);
        void SelectItem(FileBrowserItem* item);
        void DeselectItem(FileBrowserItem* item);
        void ClearSelections();
        void SortItemList();
        FileBrowserItemList Search(const std::string& content, FileBrowserDirectory* directory);
    private:
        FileBrowserDirectory* GetDirectory(const std::filesystem::path& path) const;
        const std::filesystem::path& ProcessDirectory(const std::filesystem::path& path, FileBrowserDirectory* parent);
        HorizonEditor* editor;
        AssetManager* assetManager;
        std::mutex lockMutex;
        bool isHovered;
        bool isFocused;
        bool isAnyItemHovered;
        FileBrowserItemList currentItems;
        FileBrowserSelectionStack selectedItems;
        FileBrowserSelectionStack copiedAssets;
        FileBrowserDirectory* baseDirectory;
        FileBrowserDirectory* previousDirectory;
        FileBrowserDirectory* currentDirectory;
        FileBrowserDirectory* nextDirectory;
        bool m_UpdateNavigationPath = false;
        std::vector<FileBrowserDirectory*> m_BreadCrumbData;
        std::unordered_map<std::string, FileBrowserDirectory*> directories;

        std::map<std::string, RenderBackendTextureHandle> iconMap;
        float thumbnailSize = 156.0f;
        float padding = 2.0f;
        float topBarHeight = 34.0f;
        char renameBuffer[MAX_INPUT_BUFFER_LENGTH];
        char searchBuffer[MAX_INPUT_BUFFER_LENGTH];
    };
}