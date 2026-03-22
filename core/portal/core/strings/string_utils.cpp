//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "string_utils.h"

#include <ranges>

namespace portal
{
std::string get_last_part(const std::string_view string, const char separator)
{
    const auto pos = string.rfind(separator);
    PORTAL_ASSERT(pos != std::string_view::npos, "Invalid resource id");
    return std::string(string.substr(pos + 1));
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
