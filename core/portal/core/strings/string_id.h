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

    constexpr explicit StringId(HashType id): id(id)
    {
        if consteval
        {
            string = StringRegistry::find_constexpr(id);
            if (string == INVALID_STRING_VIEW)
            {
                throw std::runtime_error("Id not found in constant registry");
            }
        }
        else
        {
            string = StringRegistry::find(id);
            if (string == INVALID_STRING_VIEW)
            {
                LOG_ERROR_TAG("StringId", "StringId with id {} not found in registry", id);
            }
        }
    }

    explicit constexpr StringId(const std::string_view string_to_calculate)
    {
        if consteval
        {
            id = StringRegistry::find_by_string_constexpr(string_to_calculate);
            string = StringRegistry::find_constexpr(id);
        }
        else
        {
            id = hash::rapidhash(string_to_calculate.data(), string_to_calculate.size());
            string = StringRegistry::store(id, string_to_calculate);
        }
    }

    StringId(HashType id, std::string_view string);
    StringId(HashType id, const std::string& string);

    bool operator==(const StringId&) const;
};

const auto INVALID_STRING_ID = StringId{uint64_t{0}, INVALID_STRING_VIEW};
const auto MAX_STRING_ID = StringId{std::numeric_limits<StringId::HashType>::max(), INVALID_STRING_VIEW};

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

#define STRING_ID(string) portal::StringId(std::string_view(string))

