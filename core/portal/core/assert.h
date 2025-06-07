//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "log.h"

#ifdef PORTAL_PLATFORM_WINDOWS
#define PORTAL_DEBUG_BREAK __debugbreak
#elif defined(PORTAL_COMPILER_CLANG)
#define PORTAL_DEBUG_BREAK __builtin_debugtrap()
#elif __APPLE__
#pragma clang diagnostic error "-Wdelete-non-abstract-non-virtual-dtor"
#pragma clang diagnostic ignored "-Wnonportable-include-path"
#pragma clang diagnostic ignored "-Wundeclared-selector"
#pragma clang diagnostic ignored "-Wswitch"

#ifdef __arm64 //arm64
#define PORTAL_DEBUG_BREAK() asm("svc 0");
#elif __clang_major__ >= 6  //XCode 6 and up intel
#define PORTAL_DEBUG_BREAK() __asm { int 3 }
#else
#define PORTAL_DEBUG_BREAK() __debugbreak()
#endif

#else
#define PORTAL_DEBUG_BREAK __debugbreak
#endif

#ifdef PORTAL_PLATFORM_WINDOWS
#define PORTAL_DEBUG_BREAK_HELPER PORTAL_DEBUG_BREAK
#else
inline void PORTAL_DEBUG_BREAK_HELPER()
{
    PORTAL_DEBUG_BREAK();
}
#endif

#ifndef PORTAL_DIST
#define PORTAL_ENABLE_ASSERTS
#endif

#ifdef PORTAL_ENABLE_ASSERTS
#define PORTAL_CORE_ASSERT_MESSAGE_INTERNAL(...)  ::portal::Log::print_assert_message(::portal::Log::LoggerType::Core,    (__FILE__), __LINE__, __FUNCTION__, __VA_ARGS__)
#define PORTAL_ASSERT_MESSAGE_INTERNAL(...)       ::portal::Log::print_assert_message(::portal::Log::LoggerType::Client, (__FILE__), __LINE__, __FUNCTION__, __VA_ARGS__)

#define PORTAL_CORE_ASSERT(condition, ...) (void)((!!(condition)) || !PORTAL_CORE_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__)  || (PORTAL_DEBUG_BREAK_HELPER(), 0))
#define PORTAL_ASSERT(condition, ...)      (void)((!!(condition)) || !PORTAL_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__)       || (PORTAL_DEBUG_BREAK_HELPER(), 0))
#else
#define PORTAL_CORE_ASSERT(condition, ...)
#define PORTAL_ASSERT(condition, ...)
#endif