//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "string_utils.h"

#include <ranges>

namespace portal
{
std::string get_last_part(std::string_view string, char separator)
{
    auto split_view = string | std::views::split(separator);
    PORTAL_ASSERT(std::ranges::distance(split_view) > 1, "Invalid resource id");

    for (auto [index, part] : split_view | std::views::enumerate)
    {
        if (index == std::ranges::distance(split_view) - 1)
            return std::string(std::string_view(part));
    }
    return "";
}

std::string to_lower_copy(const std::string_view str)
{
    std::string res(str);
    to_lower(res);
    return res;
}

std::string& to_lower(std::string& str)
{
    std::ranges::transform(str, str.begin(), [](const std::string::value_type c) { return std::tolower(c); });
    return str;
}
}
