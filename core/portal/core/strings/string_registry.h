//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory_resource>
#include <string>
#include <string_view>
#include <unordered_map>

#include <frozen/unordered_map.h>
#include "portal/core/strings/frozen_mapping.h"


namespace portal
{

constexpr static auto INVALID_STRING_VIEW = std::string_view("Invalid");

// TODO: have this not as global?
class StringRegistry
{
public:
    static constexpr uint64_t find_by_string_constexpr(const std::string_view string)
    {
        if (G_FROZEN_STRING_TO_ID.contains(string))
            return G_FROZEN_STRING_TO_ID.at(string);

        return 0;
    }

    static constexpr std::string_view find_constexpr(const uint64_t id)
    {
        if (G_FROZEN_ID_TO_STRING.contains(id))
            return G_FROZEN_ID_TO_STRING.at(id);

        return INVALID_STRING_VIEW;
    }

    static std::string_view store(uint64_t id, std::string_view string);
    static std::string_view find(uint64_t id);

private:
    static std::pmr::memory_resource* get_allocator();
    static std::pmr::unordered_map<uint64_t, std::pmr::string>& get_entries();
};
} // portal
