//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <string>
#include <portal/core/common.h>

#include "rapidhash/rapidhash.h"

/**
 * Macro controlling constexpr support for hash functions.
 *
 * On GCC/Clang: Expands to `constexpr` + force inline, enabling compile-time
 *               hash computation for string literals.
 * On MSVC:      Expands to force inline only. MSVC's constexpr implementation
 *               lacks support for 128-bit multiplication (used internally by
 *               rapidhash), preventing compile-time evaluation. Hash is still
 *               computed at runtime but highly optimized.
 *
 * TODO: Find a fully constexpr hash implementation compatible with MSVC.
 */
# if !defined(PORTAL_COMPILER_MSVC)
#   define PORTAL_HASH_CONSTEXPR PORTAL_FORCE_INLINE constexpr
# else
#   define PORTAL_HASH_CONSTEXPR PORTAL_FORCE_INLINE
# endif

/**
 * Hash function wrappers using rapidhash V3 algorithm.
 *
 * rapidhash is a high-quality, platform-independent hash function based on
 * wyhash, designed for speed and excellent avalanche properties (small input
 * changes cause large hash changes). It produces 64-bit hashes suitable for
 * hash tables and string identifiers.
 *
 * All overloads delegate to the rapidhash library from:
 * https://github.com/Nicoshev/rapidhash
 *
 * @see STRING_ID macro for compile-time string hashing
 * @see StringId for the primary consumer of these hash functions
 */
namespace portal::hash
{
/**
 * Hashes raw character data with specified length.
 *
 * This is the base overload used by all others. On GCC/Clang with constexpr
 * support, this can be evaluated at compile-time for string literals.
 *
 * @param data Pointer to character data to hash
 * @param length Number of bytes to hash (excluding null terminator if present)
 * @return 64-bit hash value
 */
PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const char* data, const size_t length)
{
    return ::rapidhash(data, length);
}

/**
 * Hashes a std::string_view.
 *
 * Delegates to the data/length overload using the string's c_str() and length().
 * Note: Not constexpr even on GCC/Clang because std::string is not a literal type.
 *
 * @param str The string to hash
 * @return 64-bit hash value
 */
PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const std::string_view str)
{
    return ::rapidhash(str.data(), str.length());
}

/**
 * Hashes a std::string.
 *
 * Delegates to the data/length overload using the string's c_str() and length().
 * Note: Not constexpr even on GCC/Clang because std::string is not a literal type.
 *
 * @param str The string to hash
 * @return 64-bit hash value
 */
PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const std::string& str)
{
    return ::rapidhash(str.c_str(), str.length());
}

/**
 * Hashes a const char* using strlen for length.
 *
 * Delegates to the data/length overload using the string's c_str() and length().
 * Note: Not constexpr even on GCC/Clang because std::string is not a literal type.
 *
 * @param str The string to hash
 * @return 64-bit hash value
 */
PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const char* str)
{
    return ::rapidhash(str, std::strlen(str));
}

/**
 * Hashes a string literal with compile-time size deduction.
 *
 * This is the overload used by STRING_ID("literal") for compile-time hashing.
 *
 * @tparam n Size of the string literal array (including null terminator)
 * @param data String literal array reference
 * @return 64-bit hash value (constexpr on GCC/Clang)
 */
template <size_t n>
PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const char (&data)[n])
{
    // Exclude null terminator from hash
    return ::rapidhash(data, n - 1);
}
}
