//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "string_registry.h"

#include <stdexcept>

namespace portal
{
std::unordered_map<uint128_t, std::string> StringRegistry::entries;

std::string_view StringRegistry::store(uint128_t id, const std::string_view string)
{
    const auto it = entries.find(id);
    if (it != entries.end())
        return it->second;

    // Saves a copy of the string in memory
    const auto& [added_it, success] = entries.emplace(id, std::string(string));

    if (!success)
        throw std::runtime_error("Failed to store string in StringIdPool");

    return std::string_view(added_it->second);
}

std::string_view StringRegistry::find(const uint128_t id)
{
    const auto it = entries.find(id);
    if (it != entries.end())
        return it->second;
    return INVALID_STRING_VIEW;
}

} // portal
