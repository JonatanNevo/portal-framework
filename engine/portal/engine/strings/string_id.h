//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <spdlog/spdlog.h>
#include "portal/core/common.h"

#include "portal/engine/strings/hash.h"
#include "portal/engine/strings/string_registry.h"

namespace portal
{
class Deserializer;
class Serializer;

struct StringId
{
    uint64_t id = 0;
    std::string_view string = INVALID_STRING_VIEW;

    StringId() = default;
    explicit StringId(uint64_t id);
    StringId(uint64_t id, std::string_view string);
    StringId(uint64_t id, const std::string& string);

    bool operator==(const StringId&) const;

    void serialize(Serializer& s) const;
    static StringId deserialize(Deserializer& d);
};

const auto INVALID_STRING_ID = StringId{uint64_t{0}, INVALID_STRING_VIEW};

} // portal

template<>
struct std::hash<portal::StringId>
{
    std::size_t operator()(const portal::StringId& id) const noexcept
    {
        return id.id;
    }
};

template <>
struct fmt::formatter<portal::StringId>
{
    static constexpr auto parse(const fmt::format_parse_context& ctx) -> decltype(ctx.begin())
    {
       return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const portal::StringId& id, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "id(\"{}\")", id.string);
    }
};

#define STRING_ID(string) portal::StringId(portal::hash::rapidhash(string), std::string_view(string))

