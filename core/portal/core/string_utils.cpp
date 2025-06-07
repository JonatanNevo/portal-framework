//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "string_utils.h"

namespace portal
{

std::vector<std::string> split(const std::string_view string, const std::string_view& delimiters)
{
    size_t first = 0;

    std::vector<std::string> result;

    while (first <= string.size())
    {
        const auto second = string.find_first_of(delimiters, first);

        if (first != second)
            result.emplace_back(string.substr(first, second - first));

        if (second == std::string_view::npos)
            break;

        first = second + 1;
    }

    return result;
}

std::vector<std::string> split(const std::string_view string, const char delimiter)
{
    return split(string, std::string(1, delimiter));
}

}
