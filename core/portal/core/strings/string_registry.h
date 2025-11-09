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
#include <frozen/string.h>

#include "portal/core/common.h"

namespace portal
{

constexpr static auto INVALID_STRING_VIEW = std::string_view("Invalid");

// TODO: have this not as global?
class StringRegistry
{
public:
    static constexpr std::string_view find_constexpr(const uint64_t id)
    {
        if (constexpr_map.contains(id))
            return constexpr_map.at(id);

        return INVALID_STRING_VIEW;
    }

    static std::string_view store(uint64_t id, std::string_view string);
    static std::string_view find(uint64_t id);

private:
    static std::pmr::memory_resource* get_allocator();
    static std::pmr::unordered_map<uint64_t, std::pmr::string>& get_entries();

    // TODO: find some constexpr mutable solution for this, or a way to automatically generate this map based on all constexpr strings
    static constexpr frozen::unordered_map<uint64_t, std::string_view, 1> constexpr_map{
        {12345, std::string_view("hello")}
    };
};
} // portal
