//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "string_pool.h"

#include <stdexcept>

namespace portal
{

StringId StringIdPool::store(const std::string_view string)
{
    auto hash = std::hash<std::string_view>{}(string);
    const auto it = entries.find(hash);
    if (it != entries.end())
        return {hash, it->second};

    // Saves a copy of the string in memory
    auto& [added_it, success] = entries.emplace(hash, std::string(string));

    if (!success)
        throw std::runtime_error("Failed to store string in StringIdPool");

    return {hash, std::string_view(added_it->second)};
}

StringId StringIdPool::find(const std::string_view string)
{
    const auto it = entries.find(std::hash<std::string_view>{}(string));
    if (it != entries.end())
        return {it->first, it->second};
    return INVALID_STRING_ID;
}

} // portal
