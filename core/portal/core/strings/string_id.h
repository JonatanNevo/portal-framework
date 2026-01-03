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

/**
 * Compile-time string identifier using 64-bit hash for efficient lookups.
 *
 * StringId enables using strings as identifiers (for resources, entities, events,
 * etc.) while maintaining performance comparable to integer keys. The design
 * philosophy is: **the hash is the identity, the string is for debugging**.
 *
 * Two StringId instances are equal if their `id` (hash) fields match. The `string`
 * field is purely for logging, debugging, and UI display - it does not participate
 * in equality or hash map operations.
 *
 * Compile-Time Hashing:
 * Use the STRING_ID() macro for string literals known at compile time. On GCC/Clang,
 * the hash is computed at compile time (constexpr) and baked into the binary,
 * resulting in zero runtime cost. On MSVC, the hash is computed at runtime due to
 * compiler limitations with constexpr, but is still highly optimized (inlined).
 *
 * Runtime Construction:
 * For runtime strings (user input, loaded data), construct using the same macro:
 * `STRING_ID(str)`. This stores the string in the global StringRegistry for lifetime 
 * management and debug lookup.
 *
 * Hash Collisions:
 * The system assumes 64-bit rapidhash collisions are astronomically unlikely (2^64
 * keyspace). There is no runtime collision detection - equality is purely hash-based.
 *
 * Example - Compile-time string IDs:
 * @code
 * // Hash computed at compile-time (on GCC/Clang)
 * constexpr auto player_id = STRING_ID("game/player");
 * constexpr auto texture_id = STRING_ID("textures/ground.png");
 *
 * // Use as resource keys
 * auto* player_mesh = resources.get<Mesh>(player_id);
 * auto* ground_tex = resources.get<Texture>(texture_id);
 * @endcode
 *
 * Example - Runtime string IDs:
 * @code
 * // Runtime string from user input
 * std::string entity_name = read_from_config();
 * StringId entity_id = STRING_ID(entity_name);
 *
 * // String is stored in registry for debugging
 * LOG_INFO("Created entity: {}", entity_id);  // Prints: Created entity: id("MyEntity")
 * @endcode
 *
 * Example - Using as hash map key:
 * @code
 * // Works with both LLVM DenseMap and std::unordered_map
 * llvm::DenseMap<StringId, Entity*> entities;
 * std::unordered_map<StringId, Texture*> textures;
 *
 * entities[STRING_ID("player")] = player_entity;
 * @endcode
 *
 * @see StringRegistry for string storage and lifetime management
 * @see hash::rapidhash for the hashing algorithm (rapidhash V3 based on wyhash)
 */
struct StringId
{
    /**
     * Type alias for the hash value (64-bit unsigned integer).
     */
    using HashType = uint64_t;

    /**
     * The 64-bit hash - this IS the identity. Equality compares only this field.
     */
    HashType id = 0;

    /**
     * Human-readable string view for debugging/display. NOT used for equality or hashing.
     * Points to either a string literal in the binary or a string in the StringRegistry.
     */
    std::string_view string = INVALID_STRING_VIEW;

    /**
     * Default constructor creating an invalid StringId (id=0, string="Invalid").
     */
    constexpr StringId() = default;

    /**
     * Constructs StringId from hash alone, looking up the string in StringRegistry.
     *
     * This constructor queries the global StringRegistry for the corresponding string.
     * If not found, logs an error and sets string to "Invalid". This is primarily
     * used during deserialization when receiving a hash from disk/network.
     *
     * @param id The 64-bit hash value
     */
    explicit StringId(HashType id);

    /**
     * Constructs StringId from hash and string, storing the string in StringRegistry.
     *
     * This stores the string in the global registry (if not already present) and
     * sets the string field to point to the registry's copy. The string persists
     * for the application's lifetime. Use this for runtime string construction.
     *
     * @param id The 64-bit hash (typically from hash::rapidhash(string))
     * @param string The string to associate with this hash
     */
    StringId(HashType id, std::string_view string);

    /**
     * Constructs StringId from hash and std::string, storing in StringRegistry.
     * @param id The 64-bit hash
     * @param string The string to associate with this hash
     */
    StringId(HashType id, const std::string& string);

    /**
     * Equality compares ONLY the hash (id field). String is ignored.
     * @param other StringId to compare against
     * @return true if hashes match, false otherwise
     */
    bool operator==(const StringId& other) const;

    void serialize(Serializer& serializer) const;
    static StringId deserialize(Deserializer& deserializer);
};

/**
 * Macro for string ID creation with hash computation.
 *
 * On GCC/Clang: Hash is computed at compile-time (constexpr) and baked into binary.
 * On MSVC: Hash is computed at runtime (but highly optimized/inlined) due to
 * limitations with constexpr 128-bit multiplication in rapidhash.
 *
 * The string_view points to the string literal in the binary's read-only data
 * section, so there's no StringRegistry storage overhead for compile-time IDs.
 *
 * @param string String literal to hash (must be a compile-time constant)
 * @return StringId with computed hash and string view to the literal
 */
#define STRING_ID(string) portal::StringId(hash::rapidhash(string), std::string_view(string))

const static auto INVALID_STRING_ID = STRING_ID("Invalid");
const static auto MAX_STRING_ID = StringId{std::numeric_limits<StringId::HashType>::max(), INVALID_STRING_VIEW};
} // portal

/**
 * LLVM DenseMap integration for StringId.
 *
 * Enables using StringId as a key in llvm::DenseMap (used throughout the Portal
 * Framework for performance). Defines empty/tombstone keys and hash function.
 *
 * The hash function mixes high and low 32 bits (id ^ (id >> 32)) for better
 * distribution in the hash table, since the upper bits of the 64-bit hash
 * contribute to the hash map's bucket selection.
 */
template <>
struct llvm::DenseMapInfo<portal::StringId>
{
    /** @return Sentinel value for empty slots in the hash table */
    static inline portal::StringId getEmptyKey()
    {
        return portal::INVALID_STRING_ID;
    }

    /** @return Sentinel value for deleted slots in the hash table */
    static inline portal::StringId getTombstoneKey()
    {
        return portal::MAX_STRING_ID;
    }

    /**
     * Computes hash value for hash table bucket selection.
     * @param Val The StringId to hash
     * @return Mixed hash value for better distribution
     */
    static unsigned getHashValue(const portal::StringId& Val)
    {
        // Mix high and low bits for better distribution
        return static_cast<unsigned>(Val.id ^ (Val.id >> 32));
    }

    /**
     * Tests equality of two StringIds.
     * @param LHS Left-hand side
     * @param RHS Right-hand side
     * @return true if IDs match
     */
    static bool isEqual(const portal::StringId& LHS, const portal::StringId& RHS)
    {
        return LHS.id == RHS.id;
    }
}; // namespace llvm

/**
 * std::hash specialization for StringId.
 *
 * Enables using StringId with std::unordered_map and other standard library
 * hash-based containers. Simply returns the id field, which is already a
 * well-distributed 64-bit hash from rapidhash.
 */
template <>
struct std::hash<portal::StringId>
{
    /**
     * Hash function operator for std::unordered_map integration.
     * @param id The StringId to hash
     * @return The 64-bit hash value (already well-distributed from rapidhash)
     */
    std::size_t operator()(const portal::StringId& id) const noexcept
    {
        return id.id;
    }
};

/**
 * fmt::formatter specialization for StringId.
 *
 * Enables formatted output with C++20 fmt::format and spdlog. Formats StringId
 * as: id("string_value")
 *
 * Example: LOG_INFO("Entity created: {}", entity_id); // Prints: Entity created: id("player")
 */
template <>
struct fmt::formatter<portal::StringId>
{
    /**
     * Parses format specification (no custom format specs supported).
     * @param ctx Parse context
     * @return Iterator to end of parsed format spec
     */
    static constexpr auto parse(const fmt::format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    /**
     * Formats StringId as id("string_value").
     * @tparam FormatContext Format context type
     * @param id The StringId to format
     * @param ctx Format context
     * @return Iterator to end of output
     */
    template <typename FormatContext>
    auto format(const portal::StringId& id, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "id(\"{}\")", id.string);
    }
};
