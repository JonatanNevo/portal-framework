//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <string>

#include <enchantum/enchantum.hpp>

#include "portal/core/debug/assert.h"

namespace portal
{
template <typename T> requires std::is_enum_v<T>
auto to_string(const T& enum_value)
{
    return enchantum::to_string(enum_value);
}

template <typename T> requires std::is_enum_v<T>
T from_string(const std::string_view& str)
{
    auto value = enchantum::cast<T>(str);
    PORTAL_ASSERT(value.has_value(), "Invalid enum value: {}", str);
    return value.value_or(T{});
}
}

template <typename T> requires std::is_enum_v<T>
struct fmt::formatter<T>
{
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const T& enum_value, FormatContext& ctx) const
    {
        return format_to(ctx.out(), "{}", portal::to_string<T>(enum_value));
    }
};
