//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "item_list.h"
#include "item.h"

namespace portal::content_browser
{
ItemList& ItemList::operator=(const ItemList& other)
{
    items = other.items;
    return *this;
}

void ItemList::clear()
{
    std::lock_guard lock(mutex);
    items.clear();
}

void ItemList::erase(StringId resource_id)
{
    const size_t index = find_item(resource_id);
    if (index == invalid_item)
    {
        LOG_WARN_TAG("Content Browser", "Tried to erase an invalid item: {}", resource_id);
        return;
    }

    std::lock_guard lock(mutex);
    const auto it = items.begin() + index;
    items.erase(it);
}

size_t ItemList::find_item(const StringId& resource_id)
{
    if (items.empty())
        return invalid_item;

    std::scoped_lock lock(mutex);
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (items[i]->get_resource_id() == resource_id)
            return i;
    }

    return invalid_item;
}
} // portal
