//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_item.h"

namespace portal::content_browser
{
std::string extract_name(StringId resource_id)
{
    auto split_view = resource_id.string | std::views::split('/');
    PORTAL_ASSERT(std::ranges::distance(split_view) > 1, "Invalid resource id");

    for (auto [index, part] : split_view | std::views::enumerate)
    {
        if (index == std::ranges::distance(split_view) - 1)
            return std::string(std::string_view(part));
    }
    return "";
}

ResourceItem::ResourceItem(const SourceMetadata& metadata, const EditorIcon icon)
    : Item(
          Type::Resource,
          metadata.resource_id,
          extract_name(metadata.resource_id),
          icon
      ),
      metadata(metadata) {}

void ResourceItem::del()
{
    // TODO: call database and delete the given resource.
    Item::del();
}

bool ResourceItem::move(std::filesystem::path& destination)
{
    // TODO: call database and move resource and update all dependencies
    return Item::move(destination);
}

void ResourceItem::on_renamed(const std::string& new_name)
{
    // TODO: call database and move resource and update all dependencies
    Item::on_renamed(new_name);
}
}
