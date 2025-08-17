//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory_resource>
#include <string>
#include <string_view>
#include <unordered_map>

#include "portal/core/common.h"

namespace portal
{

constexpr static auto INVALID_STRING_VIEW = std::string_view("Invalid");

// TODO: have this not as global?
class StringRegistry
{
public:
    static std::string_view store(uint64_t id, std::string_view string);
    static std::string_view find(uint64_t id);

private:
    static std::pmr::memory_resource* get_allocator();
    static std::pmr::unordered_map<uint64_t, std::pmr::string>& get_entries();
};
} // portal
