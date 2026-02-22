//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "directory_item.h"

#include <imgui.h>

#include "item_list.h"
#include "portal/core/files/file_system.h"

namespace portal::content_browser
{
DirectoryItem::DirectoryItem(const Reference<DirectoryInfo>& directory_info)
    : Item(
          Type::Directory,
          directory_info->id,
          directory_info->path.filename().string(),
          EditorIcon::Directory
      ),
      directory_info(directory_info) {}

void DirectoryItem::del()
{
    // ContentBrowserPanel::Get().DeleteDirectoryItem(m_DirectoryItemInfo);
}

bool DirectoryItem::move(std::filesystem::path& destination)
{
    return FileSystem::move(directory_info->path, destination);
}

void DirectoryItem::on_renamed(const std::string& new_name)
{
    auto target = directory_info->path;
    auto destination = directory_info->path.parent_path() / new_name;

    if (destination.filename() == directory_info->path.filename())
    {
        const auto tmp = directory_info->path.parent_path() / "temp";
        FileSystem::rename(directory_info->path, tmp);
        target = tmp;
    }

    if (!FileSystem::rename(target, destination))
        LOG_ERROR("Failed to rename directory {} to {}", target.string(), destination.string());
}

void DirectoryItem::update_drop(ItemList& item_list, Action& result)
{
    if (ImGui::BeginDragDropTarget())
    {
        const auto* payload = ImGui::AcceptDragDropPayload("resource_payload");
        if (payload)
        {
            const auto count = payload->DataSize / sizeof(StringId);

            for (size_t i = 0; i < count; ++i)
            {
                auto id = *(static_cast<const StringId*>(payload->Data) + i);
                const size_t index = item_list.find_item(id);
                if (index != ItemList::invalid_item)
                {
                    if (item_list[index]->move(directory_info->path))
                    {
                        result |= ActionBit::Moved;
                        item_list.erase(id);
                    }
                }
            }
        }

        ImGui::EndDragDropTarget();
    }
}
}
