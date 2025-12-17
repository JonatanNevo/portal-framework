//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <spdlog/spdlog.h>
#include <llvm/ADT/DenseMapInfo.h>

#include "portal/core/common.h"
#include "portal/core/log.h"

#include "portal/core/strings/hash.h"
#include "portal/core/strings/string_registry.h"

namespace portal
{
class Deserializer;
class Serializer;

struct StringId
{
    using HashType = uint64_t;
    HashType id = 0;
    std::string_view string = INVALID_STRING_VIEW;

    constexpr StringId() = default;
    explicit StringId(HashType id);

    StringId(HashType id, std::string_view string);
    StringId(HashType id, const std::string& string);

    bool operator==(const StringId&) const;
};

#define STRING_ID(string) portal::StringId(hash::rapidhash(string), std::string_view(string))

const static auto INVALID_STRING_ID = STRING_ID("Invalid");
const static auto MAX_STRING_ID = StringId{std::numeric_limits<StringId::HashType>::max(), INVALID_STRING_VIEW};
} // portal

template <>
struct llvm::DenseMapInfo<portal::StringId>
{
    static inline portal::StringId getEmptyKey()
    {
        return portal::INVALID_STRING_ID;
    }

    static inline portal::StringId getTombstoneKey()
    {
        return portal::MAX_STRING_ID;
    }

    static unsigned getHashValue(const portal::StringId& Val)
    {
        // Mix high and low bits for better distribution
        return static_cast<unsigned>(Val.id ^ (Val.id >> 32));
    }

    static bool isEqual(const portal::StringId& LHS, const portal::StringId& RHS)
    {
        return LHS.id == RHS.id;
    }
}; // namespace llvm

template <>
struct std::hash<portal::StringId>
{
    std::size_t operator()(const portal::StringId& id) const noexcept
    {
        return id.id;
    }
};

template <>
struct std::formatter<portal::StringId>
{
    static constexpr auto parse(const std::format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const portal::StringId& id, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "id(\"{}\")", id.string);
    }
};
