//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <fmt/format.h>

#include "portal/core/common.h"

#include "portal/engine/strings/md5_hash.h"
#include "portal/engine/strings/string_registry.h"

namespace portal
{

struct StringId
{
    uint128_t id;
    std::string_view string;

    explicit StringId(uint128_t id);
    StringId(uint128_t id, std::string_view string);
    StringId(uint128_t id, const std::string& string);

    bool operator==(const StringId&) const;
};

constexpr auto INVALID_STRING_ID = StringId{uint128_t{0}, INVALID_STRING_VIEW};

} // portal

template<>
struct std::hash<portal::StringId>
{
    std::size_t operator()(const portal::StringId& id) const noexcept
    {
        return std::hash<portal::uint128_t>()(id.id);
    }
};

template <>
struct fmt::formatter<portal::StringId>
{
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
       return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const portal::StringId& id, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "id(\"{}\")", id.string);
    }
};

#define STRING_ID(string) portal::StringId(portal::hash::md5(string), std::string_view(string))

