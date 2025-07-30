//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#ifdef PORTAL_PROFILE
#include <tracy/Tracy.hpp>

#define PORTAL_TRACE_ALLOC(ptr, size) TracyAlloc(ptr, size)
#define PORTAL_TRACE_FREE(ptr) TracyFree(ptr)
#define PORTAL_TRACE_REALLOC(old_ptr, new_ptr, size) TracyAlloc(new_ptr, size); TracyFree(old_ptr)

#define PORTAL_PROF_ZONE ZoneScoped

#define PORTAL_NAME_THREAD(name) tracy::SetThreadName(name)

#define PORTAL_PROF_LOCK(type, varname) TracyLockable(type, varname)

#else

#define PORTAL_TRACE_ALLOC(...)
#define PORTAL_TRACE_FREE(...)
#define PORTAL_TRACE_REALLOC(...)

#define PORTAL_PROF_ZONE

#define PORTAL_NAME_THREAD(...)

#define PORTAL_PROF_LOCK( type, varname ) type varname

#endif
