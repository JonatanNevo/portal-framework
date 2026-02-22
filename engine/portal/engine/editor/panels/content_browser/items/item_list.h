//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <limits>
#include <mutex>
#include <vector>

#include "portal/core/strings/string_id.h"
#include "portal/engine/reference.h"

namespace portal::content_browser
{
class Item;

class ItemList
{
public:
    constexpr static size_t invalid_item = std::numeric_limits<size_t>::max();

    std::vector<Reference<Item>> items;
    std::vector<Reference<Item>>::iterator begin() { return items.begin(); }
    std::vector<Reference<Item>>::iterator end() { return items.end(); }
    std::vector<Reference<Item>>::const_iterator begin() const { return items.begin(); }
    std::vector<Reference<Item>>::const_iterator end() const { return items.end(); }

    Reference<Item>& operator[](const size_t index) { return items[index]; }
    const Reference<Item>& operator[](const size_t index) const { return items[index]; }

    ItemList() = default;
    ItemList(const ItemList& other) : items(other.items) {}

    ItemList& operator=(const ItemList& other);

    void clear();
    void erase(StringId resource_id);

    [[nodiscard]] size_t find_item(const StringId& resource_id);

private:
    std::mutex mutex;
};
} // portal
