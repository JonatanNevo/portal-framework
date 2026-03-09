//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_item.h"

namespace portal::content_browser
{

ResourceItem::ResourceItem(const SourceMetadata& metadata, const EditorIcon icon)
    : Item(
          Type::Resource,
          metadata.resource_id,
          std::string(metadata.name.string),
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
