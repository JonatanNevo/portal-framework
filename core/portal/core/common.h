//
// Created by Jonatan Nevo on 21/03/2025.
//

#pragma once

#include "reference.h"

#include <functional>
#include  <memory>

#define BIT(x) (1u << x)

#if defined(__GNUC__)
    #if defined(__clang__)
        #define PORTAL_COMPILER_CLANG
    #else
        #define PORTAL_COMPILER_GCC
    #endif
#elif defined(_MSC_VER)
    #define PORTAL_COMPILER_MSVC
#endif

#ifdef PORTAL_COMPILER_MSVC
    #define PORTAL_FORCE_INLINE __forceinline
    #define PORTAL_EXPLICIT_STATIC static
#elif defined(__GNUC__)
    #define PORTAL_FORCE_INLINE __attribute__((always_inline)) inline
    #define PORTAL_EXPLICIT_STATIC static
#else
    #define PORTAL_FORCE_INLINE inline
    #define PORTAL_EXPLICIT_STATIC
#endif

namespace portal
{
using byte = uint8_t;
}