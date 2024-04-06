#include "FileBrowserWindow.h"

#include <imgui_internal.h>

#include <shellapi.h>

namespace HE
{
    std::string ToLower(const std::string& string)
    {
        std::string result;
        for (const auto& character : string)
        {
            result += std::tolower(character);
        }
        return result;
    }

    namespace FileSystem
    {
        static bool ShowFileInExplorer(const std::filesystem::path& path);
        static bool DeleteFileIfExists(const std::filesystem::path& filepath);
        static bool CreateDirectory(const std::filesystem::path& directory);
        static bool OpenDirectoryInExplorer(const std::filesystem::path& path);
        static bool Rename(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
    };

    bool FileSystem::ShowFileInExplorer(const std::filesystem::path& path)
    {
        std::filesystem::path absolutePath = std::filesystem::canonical(path);
        if (!std::filesystem::exists(absolutePath))
        {
            return false;
        }
        std::string cmd = std::format("explorer.exe /select,\"{0}\"", absolutePath.string());
        system(cmd.c_str());
        return true;
    }

    bool FileSystem::Rename(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
    {
        std::filesystem::rename(oldPath, newPath);
        return true;
    }

    bool FileSystem::DeleteFileIfExists(const std::filesystem::path& filepath)
    {
        if (!std::filesystem::exists(filepath))
        {
            return false;
        }
        if (std::filesystem::is_directory(filepath))
        {
            return std::filesystem::remove_all(filepath) > 0;
        }
        return std::filesystem::remove(filepath);
    }

    bool FileSystem::CreateDirectory(const std::filesystem::path& directory)
    {
        return std::filesystem::create_directories(directory);
    }

    bool FileSystem::OpenDirectoryInExplorer(const std::filesystem::path& path)
    {
        auto absolutePath = std::filesystem::canonical(path);
        if (!std::filesystem::exists(absolutePath))
        {
            return false;
        }
        ShellExecute(NULL, L"explore", absolutePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return true;
    }
}

namespace HE::UI
{
    int s_UIContextID = 0;
    uint32_t s_Counter = 0;
    char s_IDBuffer[16];

    const char* GenerateID()
    {
        _itoa_s(s_Counter++, s_IDBuffer + 2, sizeof(s_IDBuffer) - 2, 16);
        return s_IDBuffer;
    }

    void PushID()
    {
        ImGui::PushID(s_UIContextID++);
        s_Counter = 0;
    }

    void PopID()
    {
        ImGui::PopID();
        s_UIContextID--;
    }

    static void BeginPropertyGrid()
    {
        PushID();
        ImGui::Columns(2);
    }

    static void Separator()
    {
        ImGui::Separator();
    }

    static void PushItemDisabled()
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    }

    static void PopItemDisabled()
    {
        ImGui::PopItemFlag();
    }

    static void EndPropertyGrid()
    {
        ImGui::Columns(1);
        PopID();
    }

    static bool TreeNode(const std::string& id, const std::string& label, ImGuiTreeNodeFlags flags = 0)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
        {
            return false;
        }
        return ImGui::TreeNodeBehavior(window->GetID(id.c_str()), flags, label.c_str(), NULL);
    }

    static bool ImageButton(const char* stringID, ImTextureID textureID, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1))
    {
        return ImGui::ImageButtonEx(ImGui::GetID(stringID), textureID, size, uv0, uv1, bg_col, tint_col);
    }
}

namespace HE
{
    void FileBrowserWindow::SelectItem(FileBrowserItem* item)
    {
        item->isSelected = true;
        selectedItems.Add(item->GetPath());
    }

    void FileBrowserWindow::DeselectItem(FileBrowserItem* item)
    {
        item->isSelected = false;
        selectedItems.Remove(item->GetPath());
        item->isRenaming = false;
        memset(renameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
    }

    void FileBrowserWindow::ClearSelections()
    {
        for (auto& item : currentItems)
        {
            item.isSelected = false;
        }
        selectedItems.Clear();
    }

    void FileBrowserWindow::ChangeDirectory(FileBrowserDirectory* directory)
    {
        if (!directory)
        {
            return;
        }

        m_UpdateNavigationPath = true;

        currentItems.Clear();
        {
            for (const auto& [guid, subdir] : directory->subdirectories)
            {
                currentItems.Add(FileBrowserDirectory(subdir->path, directory, editor->directoryIcon));
            }
            std::vector<Guid> invalidAssets;
            for (const auto& path : directory->assets)
            {
                //const auto& assetFile = assetManager->GetAsset(guid);
                //if (!assetFile)
                //{
                //    // invalidAssets.emplace_back(assetFile->guid);
                //}
                //else
                {
                    //const auto& icon = iconMap.find(path.extension().string()) != iconMap.end() ? iconMap[path.extension().string()] : editor->fileIcon;
                    currentItems.Add(FileBrowserAsset(path, editor->fileIcon));
                }
            }
            /*for (auto invalidHandle : invalidAssets)
            {
                directory->assets.erase(std::remove(directory->assets.begin(), directory->assets.end(), invalidHandle), directory->assets.end());
            }*/
        }

        SortItemList();
        ClearSelections();

        previousDirectory = currentDirectory;
        currentDirectory = directory;
    }

void FileBrowserWindow::UpdateDropArea(FileBrowserDirectory* directory)
{
    /*if ((directory->guid != currentDirectory->guid) && ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("asset_payload");
        if (payload)
        {
            uint32 count = payload->DataSize / sizeof(Guid);
            for (uint32 i = 0; i < count; i++)
            {
                Guid guid = *(((Guid*)payload->Data) + i);
                uint32 index = currentItems.Find(guid);
                if (index != FileBrowserItemList::InvalidIndex)
                {
                    currentItems.Remove(guid);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }*/
}

void FileBrowserWindow::RenderDirectoryHierarchy(FileBrowserDirectory* directory)
{
    std::string name = directory->path.filename().string();
    std::string id = name + "_TreeNode";
    bool previousState = ImGui::TreeNodeBehaviorIsOpen(ImGui::GetID(id.c_str()));
    bool open = UI::TreeNode(id, name, ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick);
    if (open)
    {
        for (auto& [handle, child] : directory->subdirectories)
        {
            RenderDirectoryHierarchy(child);
        }
    }
    UpdateDropArea(directory);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && directory->path != currentDirectory->path)
    {
        if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.01f))
        {
            ChangeDirectory(directory);
        }
    }
    if (open)
    {
        ImGui::TreePop();
    }
}

FileBrowserItemList FileBrowserWindow::Search(const std::string& content, FileBrowserDirectory* directory)
{
    FileBrowserItemList results;
    std::string contentLowerCase = ToLower(content);

    for (auto& [guid, subdir] : directory->subdirectories)
    {
        std::string subdirName = ToLower(subdir->GetName());
        if (subdirName.find(contentLowerCase) != std::string::npos)
        {
            results.Add(std::move(FileBrowserDirectory(subdir->path, directory, editor->directoryIcon)));
        }
        FileBrowserItemList list = Search(content, subdir);
        results.items.insert(results.items.end(), list.items.begin(), list.items.end());
    }

    for (auto& guid : directory->assets)
    {
        /*const auto& assetFile = assetManager->GetAsset(guid);
        if (assetFile)
        {
            if (contentLowerCase[0] == '.')
            {
                if (assetFile->path.extension().string().find(std::string(&contentLowerCase[1])) != std::string::npos)
                {
                    const auto& icon = iconMap.find(assetFile->path.extension().string()) != iconMap.end() ? iconMap[assetFile->path.extension().string()] : editor->fileIcon;
                    results.Add(std::move(FileBrowserAsset(guid, assetFile->path, assetFile->type, icon)));
                }
            }
            else
            {
                std::string stem = ToLower(assetFile->path.stem().string());
                if (stem.find(contentLowerCase) != std::string::npos)
                {
                    const auto& icon = iconMap.find(assetFile->path.extension().string()) != iconMap.end() ? iconMap[assetFile->path.extension().string()] : editor->fileIcon;
                    results.Add(std::move(FileBrowserAsset(guid, assetFile->path, assetFile->type, icon)));
                }
            }
        }
        else
        {
            LogError(GLogger, std::format(("Failed to find asset from Asset Manager.")));
        }*/
    }

    return results;
}

void FileBrowserWindow::RenderTopBar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    if (ImGui::BeginChild("##top_bar", ImVec2(0, topBarHeight)))
    {
        if (UI::ImageButton("##back_button", editor->backwardButtonIcon.ToUnit64(), ImVec2(25, 25)) && previousDirectory->path != baseDirectory->path)
        {
            nextDirectory = currentDirectory;
            previousDirectory = currentDirectory->parent;
            ChangeDirectory(previousDirectory);
        }
        ImGui::SameLine();
        if (UI::ImageButton("##forward_button", editor->forwardButtonIcon.ToUnit64(), ImVec2(25, 25)))
        {
            ChangeDirectory(nextDirectory);
        }
        ImGui::SameLine();
        if (UI::ImageButton("##parent_button", editor->parentButtonIcon.ToUnit64(), ImVec2(25, 25)))
        {

        }
        ImGui::SameLine();
        if (UI::ImageButton("##refresh_button", editor->refreshButtonIcon.ToUnit64(), ImVec2(25, 25)))
        {
            Refresh();
        }
        ImGui::SameLine();
        if (m_UpdateNavigationPath)
        {
            m_BreadCrumbData.clear();
            FileBrowserDirectory* current = currentDirectory;
            while (current && current->parent != nullptr)
            {
                m_BreadCrumbData.push_back(current);
                current = current->parent;
            }
            std::reverse(m_BreadCrumbData.begin(), m_BreadCrumbData.end());
            m_UpdateNavigationPath = false;
        }
        std::filesystem::path path = baseDirectory->path;
        if (path.is_relative())
        {
            path = std::filesystem::absolute(path);
        }
        std::string assetsDirectoryName = path.string();

        char currentDirectoryBuffer[MAX_INPUT_BUFFER_LENGTH];
        uint32 i;
        for (i = 0; i < assetsDirectoryName.size() && i < MAX_INPUT_BUFFER_LENGTH - 2; i++)
        {
            currentDirectoryBuffer[i] = assetsDirectoryName[i];
        }
        currentDirectoryBuffer[i] = '\0';

        if (ImGui::InputText("##CurrentDirectory", currentDirectoryBuffer, MAX_INPUT_BUFFER_LENGTH))
        {
            // ChangeDirectory(baseDirectory);
        }

        float searchBarWidth = 200.0f;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - searchBarWidth);
        {
            if (ImGui::InputTextWithHint("##Search", "Search (Ctrl+F)", searchBuffer, MAX_INPUT_BUFFER_LENGTH))
            {
                if (strlen(searchBuffer) == 0)
                {
                    ChangeDirectory(currentDirectory);
                }
                else
                {
                    currentItems = Search(searchBuffer, currentDirectory);
                    SortItemList();
                }
            }
        }

        UpdateDropArea(baseDirectory);
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();
}

void FileBrowserWindow::RenderItems()
{
    static float s_Padding = 2.0f;
    const float paddingForOutline = 2.0f;
    const float scrollBarrOffset = 5.0f + ImGui::GetStyle().ScrollbarSize;
    float gridWidth = ImGui::GetContentRegionAvail().x - scrollBarrOffset;

    const float rowSpacing = 12.0f;

    const ImVec2 initTopLeft = ImGui::GetCursorScreenPos();

    const float edgeOffset = 4.0f;
    const float textLineHeight = 0.5f * (ImGui::GetTextLineHeightWithSpacing() * 2.0f + edgeOffset * 2.0f) + 2.0f;

    const float marginHorizontal = 25.0f;
    const float marginTop = 40.0f;
    const float marginBottom = 30.0f;
    const ImVec2 cellSize = ImVec2(thumbnailSize + 2.0f * marginHorizontal, thumbnailSize + textLineHeight + marginTop + marginBottom);
    const ImVec2 padding = ImVec2(12.0f, 12.0f);

    int columnCount = std::max((int)std::floor((gridWidth - padding.x) / (cellSize.x + padding.x)), 1);
    int rowCount = Math::CeilDiv((uint32)currentItems.items.size(), columnCount);

    float gridHeight = (cellSize.y + padding.y) * rowCount + padding.y;

    //ImGui::Columns(columnCount, 0, false);

    isAnyItemHovered = false;
    std::lock_guard<std::mutex> lock(lockMutex);

    Vector2u maxCellIndex = Vector2u(std::max(rowCount, 0), std::max(columnCount, 0));

    float scrollY = ImGui::GetScrollY();
    uint32 rowStart = (uint32)std::floor(scrollY / (cellSize.y + padding.y));
    uint32 rowEnd = rowStart + (uint32)std::ceil(std::max(ImGui::GetContentRegionAvail().y, 0.0f) / (cellSize.y + padding.y));
    printf("%f %f start: %u, end: %u\n", ImGui::GetContentRegionAvail().y, (cellSize.y + padding.y), rowStart, rowEnd);

    ImGui::Dummy(ImVec2(gridWidth, gridHeight));

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(paddingForOutline, rowSpacing));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7255f, 0.8314f, 0.949f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7255f, 0.8314f, 0.949f, 1.0f));

    for (uint32 cellIndexX = rowStart; cellIndexX < rowEnd; cellIndexX++)
    {
        for (uint32 cellIndexY = 0; cellIndexY < maxCellIndex.y; cellIndexY++)
        {
            uint32 itemIndex = cellIndexX * columnCount + cellIndexY;
            if (itemIndex >= currentItems.items.size())
            {
                break;
            }

            auto& item = currentItems.items[itemIndex];

            FileBrowserActionFlags result = FileBrowserActionFlags::None;

            ImGuiID itemID = ImGui::GetID(item.GetName().c_str());
            ImGui::PushID(itemID);
            //ImGui::BeginGroup();

            const ImVec2 topLeft = initTopLeft + padding + (padding + cellSize) * ImVec2(float(cellIndexY), float(cellIndexX));
            const ImVec2 bottomRight = { topLeft.x + cellSize.x, topLeft.y + cellSize.y };

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            //if (ImGui::ItemHoverable(ImRect(topLeft, bottomRight), itemID, ImGuiItemFlags_None))
            {
                //drawList->AddRectFilled(topLeft, bottomRight, IM_COL32(185, 212, 242, 255), 6.0f);
            }

            if (item.IsSelected())
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7255f, 0.8314f, 0.949f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7255f, 0.8314f, 0.949f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7255f, 0.8314f, 0.949f, 1.0f));
            }

            ImGui::SetCursorScreenPos(topLeft);
            if (ImGui::Button("##ThumbnailButton", bottomRight - topLeft))
            {
                result |= FileBrowserActionFlags::Selected;
            }

            if (item.IsSelected())
            {
                ImGui::PopStyleColor(3);
            }

            ImVec2 iconTopLeft = topLeft + ImVec2(marginHorizontal, marginTop);
            drawList->AddImage(item.GetIcon().ToUnit64(), iconTopLeft, iconTopLeft + ImVec2(thumbnailSize, thumbnailSize), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32(255, 255, 255, 255));

            if (item.GetType() == FileBrowserItemType::Directory && !item.IsSelected())
            {
                if (ImGui::BeginDragDropTarget())
                {
                    //const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("asset_payload");
                    //if (payload)
                    //{
                    //    uint32 count = payload->DataSize / sizeof(Guid);
                    //    for (uint32 i = 0; i < count; i++)
                    //    {
                    //        Guid guid = *(((Guid*)payload->Data) + i);
                    //        uint32 index = currentItems.Find(guid);
                    //        if (index != FileBrowserItemList::InvalidIndex)
                    //        {
                    //            /*if (currentItems[index]->Move(m_DirectoryInfo->FilePath))
                    //            {
                    //                actionResult.Set(FileBrowserAction::Refresh, true);
                    //                currentItems.erase(assetHandle);
                    //            }*/
                    //        }
                    //    }
                    //}
                    ImGui::EndDragDropTarget();
                }
            }


            bool dragging = false;
            if (dragging = ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                item.isDragging = true;
                if (!selectedItems.Contains(item.GetPath()))
                {
                    result |= FileBrowserActionFlags::ClearSelections;
                }
                if (selectedItems.GetCount() > 0)
                {
                    for (const auto& guid : selectedItems)
                    {
                        uint32 index = currentItems.Find(guid);
                        if (index == FileBrowserItemList::InvalidIndex)
                        {
                            continue;
                        }
                        const auto& item = currentItems[index];
                        // UI::Image(item.GetIcon(), ImVec2(20, 20));
                        ImGui::SameLine();
                        const auto& name = item.GetName();
                        ImGui::TextUnformatted(name.c_str());
                    }
                    ImGui::SetDragDropPayload("asset_payload", selectedItems.GetData(), sizeof(Guid) * selectedItems.GetCount());
                }
                result |= FileBrowserActionFlags::Selected;
                ImGui::EndDragDropSource();
            }

            //if (ImGui::IsItemHovered())
            //{
            //    result |= FileBrowserActionFlags::Hovered;
            //    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            //    {
            //        // Activate(result);
            //    }
            //    else
            //    {
            //        bool action = selectedItems.GetCount() > 1 ? ImGui::IsMouseReleased(ImGuiMouseButton_Left) : ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            //        bool skipBecauseDragging = item.isDragging && selectedItems.Contains(item.GetGuid());
            //        if (action && !skipBecauseDragging)
            //        {
            //            result |= FileBrowserActionFlags::Selected;
            //            if (!Input::IsKeyPressed(KeyCode::LeftControl) && !Input::IsKeyPressed(KeyCode::LeftShift))
            //            {
            //                result |= FileBrowserActionFlags::ClearSelections;
            //            }
            //            if (Input::IsKeyPressed(KeyCode::LeftShift))
            //            {
            //                result |= FileBrowserActionFlags::SelectToHere;
            //            }
            //        }
            //    }
            //}

            if (ImGui::BeginPopupContextItem("FileBrowserItemContextMenu"))
            {
                result |= FileBrowserActionFlags::Selected;
                if (ImGui::MenuItem("Open", "Ctrl+Shift+O"))
                {

                }
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "Ctrl+X"))
                {

                }
                if (ImGui::MenuItem("Copy", "Ctrl+C"))
                {

                }
                if (ImGui::MenuItem("Paste", "Ctrl+V"))
                {

                }
                if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
                {

                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {

                }
                if (ImGui::MenuItem("Rename", "F2"))
                {
                    StartRenamingItem(&item);
                }
                if (ImGui::MenuItem("Delete", "Delete"))
                {
                    result |= FileBrowserActionFlags::DeleteSelectedItems;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Show In Explorer"))
                {
                    result |= FileBrowserActionFlags::ShowInExplorer;
                }
                if (ImGui::MenuItem("Open Externally"))
                {
                    result |= FileBrowserActionFlags::OpenExternally;
                }
                // item.RenderCustomContextItems();
                ImGui::EndPopup();
            }

            if (!item.isRenaming)
            {
                ImVec2 textPos = ImVec2(topLeft.x + 0.5f * (bottomRight.x - topLeft.x) - 0.5f * ImGui::CalcTextSize(item.GetName().c_str()).x, bottomRight.y - textLineHeight);
                drawList->AddText(textPos, IM_COL32(0, 0, 0, 255), item.GetName().c_str());
                /*
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (thumbnailSize - edgeOffset * 3.0f));
                const float textWidth = std::min(ImGui::CalcTextSize(item.GetName().c_str()).x, thumbnailSize);
                ImGui::SetNextItemWidth(textWidth);
                ImGui::Text(item.GetName().c_str());
                ImGui::PopTextWrapPos();
                */

                if (Input::GetKeyDown(KeyCode::F2) && item.IsSelected())
                {
                    StartRenamingItem(&item);
                }
            }
            else
            {
                ImGui::SetKeyboardFocusHere();
                if (!item.IsSelected() || ImGui::InputText("##rename", renameBuffer, MAX_INPUT_BUFFER_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    const auto& newPath = item.path.parent_path() / renameBuffer;
                    if (std::filesystem::exists(newPath))
                    {
                        LogError(GLogger, std::format(("Already exists!")));
                        item.isRenaming = false;
                    }
                    else
                    {
                        FileSystem::Rename(item.path, newPath);
                        item.name = renameBuffer;
                        item.isRenaming = false;
                        ProcessDirectory(baseDirectory->path, nullptr);
                        ChangeDirectory(currentDirectory);
                        result |= FileBrowserActionFlags::Renamed;
                    }
                }
            }
            item.isDragging = dragging;

            if (ImGui::IsItemHovered())
            {
                result |= FileBrowserActionFlags::Hovered;

                bool action = (selectedItems.GetCount() > 1) ? ImGui::IsMouseReleased(ImGuiMouseButton_Left) : ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                bool skipBecauseDragging = dragging;

                if (action && !skipBecauseDragging)
                {
                    if (Input::GetKeyDown(KeyCode::LeftControl))
                    {
                        result |= FileBrowserActionFlags::Deselected;
                    }

                    if (!item.IsSelected())
                    {
                        result |= FileBrowserActionFlags::Selected;
                    }

                    if (!Input::GetKeyDown(KeyCode::LeftControl) && !Input::GetKeyDown(KeyCode::LeftShift))
                    {
                        result |= FileBrowserActionFlags::ClearSelections;
                    }

                    if (Input::GetKeyDown(KeyCode::LeftShift))
                    {
                        result |= FileBrowserActionFlags::SelectToHere;
                    }
                }
            }

            //ImGui::EndGroup();

            if (HAS_ANY_FLAGS(result, FileBrowserActionFlags::ClearSelections))
            {
                ClearSelections();
            }
            if (HAS_ANY_FLAGS(result, FileBrowserActionFlags::Selected) && !selectedItems.Contains(item.GetPath()))
            {
                SelectItem(&item);
            }
            if (HAS_ANY_FLAGS(result, FileBrowserActionFlags::Deselected) && selectedItems.Contains(item.GetPath()))
            {
                DeselectItem(&item);
            }
            if (HAS_ANY_FLAGS(result, FileBrowserActionFlags::SelectToHere) && selectedItems.GetCount() == 2)
            {
                uint32 firstIndex = currentItems.Find(selectedItems[0]);
                uint32 lastIndex = currentItems.Find(item.GetPath());
                if (firstIndex > lastIndex)
                {
                    uint32 temp = firstIndex;
                    firstIndex = lastIndex;
                    lastIndex = temp;
                }
                for (uint32 i = firstIndex + 1; i < lastIndex; i++)
                {
                    SelectItem(&currentItems[i]);
                }
            }
            if (HAS_ANY_FLAGS(result, FileBrowserActionFlags::ShowInExplorer))
            {
                if (item.GetType() == FileBrowserItemType::Directory)
                {
                    FileSystem::OpenDirectoryInExplorer(item.GetPath());
                }
                else
                {
                    FileSystem::ShowFileInExplorer(item.GetPath());
                }
            }
            if (HAS_ANY_FLAGS(result, FileBrowserActionFlags::Hovered))
            {
                isAnyItemHovered = true;
            }

            if (HAS_ANY_FLAGS(result, FileBrowserActionFlags::DeleteSelectedItems))
            {
                DeleteSelectedItems();
                break;
            }
            if (HAS_ANY_FLAGS(result, FileBrowserActionFlags::Renamed))
            {
                SortItemList();
                break;
            }
            if (HAS_ANY_FLAGS(result, FileBrowserActionFlags::ChangeDirectory))
            {
                ChangeDirectory((FileBrowserDirectory*)&item);
                break;
            }
            if (HAS_ANY_FLAGS(result, FileBrowserActionFlags::Refresh))
            {
                Refresh();
                break;
            }

            ImGui::PopID();

            //cellIndex.x += 1;
            //if (cellIndex.x >= columnCount)
            //{
            //    // TODO: We can only process items which can be seen to optimize performance when there are a lot of items.
            //    // eg, add an big invisible button to make scroller work.

            //    // Break if out of window!
            //    if ((bottomRight.y + padding.y) > (initTopLeft.y + ImGui::GetContentRegionAvail().y))
            //    {
            //        break;
            //    }
            //    cellIndex.x = 0;
            //    cellIndex.y++;
            //}
            //else
            //{
            //    ImGui::SameLine();
            //}
        }
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(4);
}

void FileBrowserWindow::RenderBottomBar()
{
    ImGui::BeginChild("##panel_controls", ImVec2(ImGui::GetColumnWidth() - 12, 30), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::Separator();
        ImGui::Columns(4, 0, false);
        if (selectedItems.GetCount() == 1)
        {
            /*std::string path;
            const auto& assetFile = assetManager->GetAsset<Asset>(selectedItems[0].);
            if (assetFile)
            {
                path = assetFile->filename;
            }
            else if (directories.find(selectedItems[0]) != directories.end())
            {
                path = directories[selectedItems[0]]->path.string();
                std::replace(path.begin(), path.end(), '\\', '/');
            }
            ImGui::Text(path.c_str());*/
        }
        else if (selectedItems.GetCount() > 1)
        {
            ImGui::Text("%d items selected", selectedItems.GetCount());
        }
        ImGui::NextColumn();
        ImGui::NextColumn();
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
        ImGui::SliderFloat("##thumbnail_size", &thumbnailSize, 32.0f, 96.0f);
    }
    ImGui::EndChild();
}

void FileBrowserWindow::OnImGuiRender()
{
    if (ImGui::Begin("File Browser", NULL, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
    {
        isHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
        isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10.0f, 2.0f));

        UI::PushID();

        ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV;
        if (ImGui::BeginTable(UI::GenerateID(), 2, tableFlags, ImVec2(0.0f, 0.0f)))
        {
            ImGui::TableSetupColumn("Outliner", 0, 300.0f);
            ImGui::TableSetupColumn("Directory Structure", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            if (ImGui::BeginChild("##folders_common"))
            {
                bool open = ImGui::CollapsingHeader("Content", nullptr, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick);
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && baseDirectory->path != currentDirectory->path)
                {
                    if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.01f))
                    {
                        ChangeDirectory(baseDirectory);
                    }
                }
                if (open)
                {
                    for (auto& [handle, directory] : baseDirectory->subdirectories)
                    {
                        RenderDirectoryHierarchy(directory);
                    }
                }

            }
            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);

            if (ImGui::BeginChild("##directory_structure", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetWindowHeight() - topBarHeight - 20.0f)))
            {
                RenderTopBar();

                ImGui::Separator();

                if (ImGui::BeginChild("Scrolling", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
                    if (ImGui::BeginPopupContextWindow())
                    {
                        if (ImGui::BeginMenu("New"))
                        {
                            if (ImGui::MenuItem("Folder"))
                            {
                                const auto& path = currentDirectory->path / "New Folder";
                                bool created = FileSystem::CreateDirectory(path);
                                if (created)
                                {
                                    const auto& directory = new FileBrowserDirectory(path, currentDirectory, editor->directoryIcon);
                                    directories[directory->path.string()] = directory;
                                    auto& newFolder = currentItems.Add(*directory);
                                    StartRenamingItem(&newFolder);
                                    ClearSelections();
                                    SelectItem(&newFolder);
                                    SortItemList();
                                }
                            }
                            if (ImGui::MenuItem("Scene"))
                            {
                                //
                            }
                            if (ImGui::MenuItem("Material"))
                            {
                                //
                            }
                            ImGui::EndMenu();
                        }
                        if (ImGui::MenuItem("Refresh"))
                        {
                            Refresh();
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Show in Explorer"))
                        {
                            FileSystem::OpenDirectoryInExplorer(currentDirectory->path);
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::PopStyleVar();

                    RenderItems();

                    if (ImGui::IsWindowFocused() && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                    {
                        UpdateInput();
                    }
                    //ImGui::PopStyleColor(2);
                }
                ImGui::EndChild();
            }
            ImGui::EndChild();

            if (ImGui::BeginDragDropTarget())
            {
                auto data = ImGui::AcceptDragDropPayload("scene_hierarchy");
                if (data)
                {
                    //
                }
                ImGui::EndDragDropTarget();
            }
            // RenderBottomBar();
        }
        ImGui::EndTable();

        UI::PopID();

        ImGui::PopStyleVar(3);
    }
    ImGui::End();
}

void FileBrowserWindow::DeleteSelectedItems()
{
    for (std::filesystem::path guid : selectedItems)
    {
        uint32 index = currentItems.Find(guid);
        if (index == FileBrowserItemList::InvalidIndex)
        {
            continue;
        }
        auto& item = currentItems[index];
        bool deleted = FileSystem::DeleteFileIfExists(item.path);
        if (!deleted)
        {
            // LogError(GLogger, std::format(("Failed to delete {}", item.path.c_str())));
            return;
        }
        switch (item.GetType())
        {
        case FileBrowserItemType::Directory:
            break;
        case FileBrowserItemType::File:
            break;
        }
        currentItems.Remove(guid);
    }
    ProcessDirectory(baseDirectory->path, nullptr);
    ChangeDirectory(currentDirectory);
}

void FileBrowserWindow::SortItemList()
{
    std::sort(currentItems.begin(), currentItems.end(),
        [](const FileBrowserItem& item1, const FileBrowserItem& item2)
        {
            if (item1.GetType() == item2.GetType())
            {
                return ToLower(item1.GetName()) < ToLower(item2.GetName());
            }
            return (uint32)item1.GetType() < (uint32)item2.GetType();
        });
}

FileBrowserDirectory* FileBrowserWindow::GetDirectory(const std::filesystem::path& path) const
{
    for (const auto& [guid, directory] : directories)
    {
        if (directory->path.string() == path.string())
        {
            return directory;
        }
    }
    return nullptr;
}

const std::filesystem::path& FileBrowserWindow::ProcessDirectory(const std::filesystem::path& path, FileBrowserDirectory* parent)
    {
        FileBrowserDirectory* directory = GetDirectory(path);
        if (directory)
        {
            directory->assets.clear();
            directory->subdirectories.clear();
        }
        else
        {
            directory = new FileBrowserDirectory(path, parent, editor->directoryIcon);
        }
        for (auto entry : std::filesystem::directory_iterator(path))
        {
            if (entry.is_directory())
            {
                const std::filesystem::path& guid = ProcessDirectory(entry.path(), directory);
                directory->subdirectories[guid.string()] = directories[guid.string()];
            }
            else
            {
                const auto& asset = AssetManager::GetAsset<Asset>(entry.path().string());
                if (!asset)
                {
                    AssetManager::ImportAsset(entry.path());
                    directory->assets.push_back(entry.path().string());
                }
                else
                {
                    directory->assets.push_back(entry.path().string());
                }
            }
        }
        directories[directory->path.string()] = directory;
        return directory->path;
    }

    void FileBrowserWindow::Refresh()
    {
        ProcessDirectory(baseDirectory->path, nullptr);
        ChangeDirectory(currentDirectory);
    }

    void FileBrowserWindow::UpdateInput()
    {
        if (!isHovered)
        {
            return;
        }

        if (!isAnyItemHovered && ImGui::IsAnyMouseDown())
        {
            ClearSelections();
        }

        if (Input::GetKeyDown(KeyCode::Delete))
        {
            DeleteSelectedItems();
        }

        if (Input::GetKeyDown(KeyCode::F5))
        {
            Refresh();
        }
    }

    void FileBrowserWindow::RenameSelectedItems()
    {

    }

    void FileBrowserWindow::StartRenamingItem(FileBrowserItem* item)
    {
        if (item->isRenaming)
        {
            return;
        }
        memset(renameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
        memcpy(renameBuffer, item->name.c_str(), item->name.size());
        item->isRenaming = true;
    }

    FileBrowserWindow::FileBrowserWindow(HorizonEditor* editor, AssetManager* assetManager)
        : editor(editor)
        , assetManager(assetManager)
    {
        std::filesystem::path path = ProcessDirectory("../../../Assets", nullptr);
        baseDirectory = directories[path.string()];
        ChangeDirectory(baseDirectory);

        memset(searchBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
    }

    FileBrowserWindow::~FileBrowserWindow()
    {

    }
}
