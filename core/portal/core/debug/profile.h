//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

// TODO: profile zone should also use performance timers
#ifdef PORTAL_PROFILE
#include <tracy/Tracy.hpp>

#define PORTAL_TRACE_ALLOC(ptr, size) TracyAlloc(ptr, size)
#define PORTAL_TRACE_FREE(ptr) TracyFree(ptr)
#define PORTAL_TRACE_REALLOC(old_ptr, new_ptr, size) TracyAlloc(new_ptr, size); TracyFree(old_ptr)

#define PORTAL_PROF_ZONE_FUNC(...) ZoneScoped##__VA_OPT__(N(__VA_ARGS__))
#define PORTAL_PROF_ZONE(...) PORTAL_PROF_ZONE_FUNC(__VA_ARGS__)

#define PORTAL_NAME_THREAD(name) tracy::SetThreadName(name)

#define PORTAL_PROF_LOCK(type, varname) TracyLockable(type, varname)

#else

#define PORTAL_TRACE_ALLOC(...)
#define PORTAL_TRACE_FREE(...)
#define PORTAL_TRACE_REALLOC(...)

#define PORTAL_PROF_ZONE(...)

#define PORTAL_NAME_THREAD(...)

#define PORTAL_PROF_LOCK( type, varname ) type varname

#endif
