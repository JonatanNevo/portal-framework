//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/assert.h"
#include <mutex>

namespace portal
{
/**
 * A "good enough" low cost lock that should be used for critical section, and stripped out in production.
 * Used *only* for development
 */
class AssertionLock
{
public:
    AssertionLock() : locked(false) {}

    void lock()
    {
        // Assert no one already has the lock
        PORTAL_ASSERT(!locked, "Cannot lock an already locked AssertionLock");
        locked = true;
    }

    void unlock()
    {
        // assert correct usage
        PORTAL_ASSERT(locked, "Cannot unlock an already unlocked AssertionLock");
        locked = false;
    }

private:
    volatile bool locked;
};

#ifdef PORTAL_ENABLE_ASSERTS
#define ASSERT_LOCK(L) L.lock()
#define ASSERT_UNLOCK(L) L.unlock()
#define ASSERT_LOCK_GUARD(L) std::lock_guard<AssertionLock> guard(L);
#else
#define ASSERT_LOCK(L)
#define ASSERT_UNLOCK(L)
#define ASSERT_LOCK_GUARD(L)
#endif
}
