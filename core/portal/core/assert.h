//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once

#include "log.h"

#ifdef PORTAL_PLATFORM_WINDOWS
#define PORTAL_DEBUG_BREAK __debugbreak()
#elif defined(PORTAL_COMPILER_CLANG)
#define PORTAL_DEBUG_BREAK __builtin_debugtrap()
#else
#define PORTAL_DEBUG_BREAK
#endif

#ifdef PORTAL_DEBUG
#define PORTAL_ENABLE_ASSERTS
#endif

#define PORTAL_ENABLE_VERIFY

#ifdef PORTAL_ENABLE_ASSERTS
#define PORTAL_CORE_ASSERT_MESSAGE_INTERNAL(...) ::portal::Log::print_assert_message(::portal::Log::Type::Core, "Assertion Failed", __VA_ARGS__)
#define PORTAL_ASSERT_MESSAGE_INTERNAL(...) ::portal::Log::print_assert_message(::portal::Log::Type::Client, "Assertion Failed", __VA_ARGS__)

#define PORTAL_CORE_ASSERT(condition, ...) {if(!(condition)) {PORTAL_CORE_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); PORTAL_DEBUG_BREAK;}}
#define PORTAL_ASSERT(condition, ...) {if(!(condition)) {PORTAL_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); PORTAL_DEBUG_BREAK;}}
#else
#define PORTAL_CORE_ASSERT(condition, ...)
#define PORTAL_ASSERT(condition, ...)
#endif

#ifdef PORTAL_ENABLE_VERIFY
#define PORTAL_CORE_VERIFY_MESSAGE_INTERNAL(...) ::portal::Log::print_assert_message(::portal::Log::Type::Core, "Verify Failed", __VA_ARGS__)
#define PORTAL_VERIFY_MESSAGE_INTERNAL(...) ::portal::Log::print_assert_message(::portal::Log::Type::Client, "Verify Failed", __VA_ARGS__)

#define PORTAL_CORE_VERIFY(condition, ...) {if(!(condition)) {PORTAL_CORE_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); PORTAL_DEBUG_BREAK;}}
#define PORTAL_VERIFY(condition, ...) {if(!(condition)) {PORTAL_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); PORTAL_DEBUG_BREAK;}}
#else
#define PORTAL_CORE_VERIFY(condition, ...)
#define PORTAL_VERIFY(condition, ...)
#endif