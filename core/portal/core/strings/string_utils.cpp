//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "string_utils.h"

namespace portal
{
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
