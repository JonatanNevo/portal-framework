//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "portal/core/common.h"

namespace portal
{

constexpr static auto INVALID_STRING_ID = std::string_view("Invalid");

// TODO: have this not as global?
class StringRegistry
{
public:
    static std::string_view store(uint128_t id, const std::string_view string);
    static std::string_view find(const uint128_t id);

private:
    static std::unordered_map<uint128_t, std::string> entries;
};
} // portal
