//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/common.h"


#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#  define PORTAL_CPU_RELAX_X86 1
#  include <immintrin.h>
#elif defined(PORTAL_COMPILER_MSVC) && (defined(_M_ARM) || defined(_M_ARM64))
#  define PORTAL_CPU_RELAX_MSVC_ARM 1
#  include <intrin.h>
#elif (defined(PORTAL_COMPILER_GCC) || defined(PORTAL_COMPILER_CLANG)) \
    && (defined(__aarch64__) || defined(__arm__) || defined(__ARM_ARCH))
#  define PORTAL_CPU_RELAX_ARM 1
#else
#  define PORTAL_CPU_RELAX_FALLBACK 1
#  include <thread>
#endif

namespace portal
{

PORTAL_FORCE_INLINE void cpu_relax()
{
#if defined(PORTAL_CPU_RELAX_X86)
    _mm_pause();
#elif defined(PORTAL_CPU_RELAX_MSVC_ARM)
    __yield();
#elif defined(PORTAL_CPU_RELAX_ARM)
    __asm__ __volatile__("yield" ::: "memory");
#else
    std::this_thread::yield();
#endif
}

}
