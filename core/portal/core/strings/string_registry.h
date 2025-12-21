//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory_resource>
#include <string>
#include <string_view>
#include <unordered_map>

#include <frozen/unordered_map.h>


namespace portal
{
/**
 * Sentinel value for invalid or not-found string IDs.
 */
constexpr auto INVALID_STRING_VIEW = std::string_view("Invalid");

/**
 * Global singleton registry for StringId string storage and lifetime management.
 *
 * StringRegistry maintains a permanent mapping of 64-bit hashes to their associated
 * strings. When a StringId is constructed with a runtime string (via the constructor
 * taking both hash and string), the string is stored in this registry and persists
 * for the application's entire lifetime.
 *
 * Architecture:
 * The registry uses Meyer's singleton pattern (static local in get_entries()) and
 * employs std::pmr (polymorphic memory resources) for efficient string storage:
 * - monotonic_buffer_resource (64KB) for fast bulk allocation
 * - unsynchronized_pool_resource on top for variable-sized string allocations
 * - Strings are never deallocated (monotonic buffer only grows)
 *
 * This design is optimal for StringId usage patterns: strings are typically
 * registered once at startup or load time, then referenced many times. The
 * monotonic buffer avoids allocation fragmentation and deallocation overhead.
 *
 * Thread Safety:
 * NOT thread-safe. String registration (store()) and lookup (find()) are not
 * protected by locks. However, this is typically acceptable because:
 * - Most string registration happens during initialization (single-threaded)
 * - Compile-time STRING_ID() macros don't use the registry at all
 * - Runtime StringId construction is usually at load time, not hot paths
 *
 * If concurrent access is required, external synchronization must be used.
 *
 * Usage:
 * You rarely call StringRegistry methods directly - StringId constructors handle
 * it automatically. The registry is an implementation detail of the StringId system.
 *
 * @see StringId for the main string identifier interface
 */
class StringRegistry
{
public:
    /**
     * Stores a string in the registry, associating it with a hash.
     *
     * If the hash already exists in the registry, returns the existing stored
     * string view (deduplication). If new, allocates a std::pmr::string using
     * the registry's memory resources and stores it permanently.
     *
     * The returned string_view points into the registry's storage and remains
     * valid for the application's lifetime.
     *
     * @param id The 64-bit hash of the string (typically from hash::rapidhash)
     * @param string The string to store
     * @return string_view pointing to the registry's copy of the string
     */
    static std::string_view store(uint64_t id, std::string_view string);

    /**
     * Looks up a string by its hash in the registry.
     *
     * Returns the stored string if found, or INVALID_STRING_VIEW if the hash
     * is not in the registry. Used by StringId's hash-only constructor during
     * deserialization.
     *
     * @param id The 64-bit hash to look up
     * @return string_view to the stored string, or INVALID_STRING_VIEW if not found
     */
    static std::string_view find(uint64_t id);

private:
    static std::pmr::memory_resource* get_allocator();
    static std::pmr::unordered_map<uint64_t, std::pmr::string>& get_entries();
};
} // portal
