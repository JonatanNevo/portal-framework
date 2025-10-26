//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

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
