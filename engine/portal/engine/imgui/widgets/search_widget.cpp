//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "search_widget.h"

namespace portal::imgui
{
bool is_matching_search(
    const std::string_view item,
    const std::string_view search_query,
    const bool case_sensitive,
    const bool strip_white_spaces,
    const bool strip_underscores
)
{
    if (search_query.empty())
        return true;

    if (item.empty())
        return false;

    auto item_sanitized = std::string(item);
    if (strip_underscores)
        std::ranges::replace(item_sanitized, '_', ' ');
    if (strip_white_spaces)
        item_sanitized = std::ranges::to<std::string>(std::ranges::remove(item_sanitized, ' '));

    auto query_sanitized = std::string(search_query);
    if (strip_white_spaces)
        query_sanitized = std::ranges::to<std::string>(std::ranges::remove(query_sanitized, ' '));

    if (!case_sensitive)
    {
        item_sanitized = to_lower(item_sanitized);
        query_sanitized = to_lower(query_sanitized);
    }

    bool result = false;
    if (query_sanitized.contains(" "))
    {
        for (const auto& term : query_sanitized | std::views::split(' '))
        {
            if (!term.empty() && item_sanitized.contains(std::string_view(term)))
            {
                result = true;
            }
            else
            {
                result = false;
                break;
            }
        }
    }
    else
    {
        result = item_sanitized.contains(query_sanitized);
    }

    return result;
}
}
