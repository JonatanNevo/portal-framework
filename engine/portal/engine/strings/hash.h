//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <string>
#include <portal/core/common.h>

// skips non constexpr intrinsics in windows
#include "rapidhash/rapidhash.h"

// MSVC consteval does not support reinterpret cast correctly, therefore the hash cannot be constexpr
# if !defined(PORTAL_COMPILER_MSVC)
 #   define PORTAL_HASH_CONSTEXPR PORTAL_FORCE_INLINE constexpr
 # else
 #   define PORTAL_HASH_CONSTEXPR PORTAL_FORCE_INLINE
 # endif

namespace portal::hash
{

PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const char* data, const size_t length)
{
    return ::rapidhash(data, length);
}

PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const std::string& str)
{
    return portal::hash::rapidhash(str.c_str(), str.length());
}

template <size_t n>
PORTAL_HASH_CONSTEXPR uint64_t rapidhash(const char (&data)[n])
{
    // Exclude null terminator
    return portal::hash::rapidhash(data, n - 1);
}

}
