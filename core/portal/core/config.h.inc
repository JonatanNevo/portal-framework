//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <string_view>
#include "portal/core/common.h"

#if defined(PORTAL_DIST) && defined(PORTAL_DEVELOPMENT)
#undef PORTAL_DIST
#endif

/* Platform Specification */
#cmakedefine PORTAL_PLATFORM_WINDOWS
#cmakedefine PORTAL_PLATFORM_MACOS
#cmakedefine PORTAL_PLATFORM_LINUX

#define WINDOWS windows
#define MACOS   macos
#define LINUX   linux
#cmakedefine PORTAL_PLATFORM @PORTAL_PLATFORM@

/* Define if profiling is enabled */
#cmakedefine PORTAL_PROFILE

/* Enable to debug allocations */
#ifndef PORTAL_DIST
#cmakedefine PORTAL_DEBUG_ALLOCATIONS
#endif

namespace portal
{

extern const std::string_view PORTAL_APPLICATION_NAME;

}