//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "portal/core/concurrency/spin_lock.h"

using namespace portal;

TEST_CASE("SpinLock try_lock contract", "[spinlock]")
{
    SpinLock lock;

    SECTION("acquires when free")
    {
        REQUIRE(lock.try_lock());
        lock.unlock();
    }

    SECTION("fails while held, succeeds after unlock")
    {
        REQUIRE(lock.try_lock());
        REQUIRE_FALSE(lock.try_lock());
        lock.unlock();
        REQUIRE(lock.try_lock());
        lock.unlock();
    }
}

TEST_CASE("SpinLock works with std::lock_guard", "[spinlock]")
{
    SpinLock lock;
    {
        std::lock_guard guard(lock);
        REQUIRE_FALSE(lock.try_lock());
    }
    // released after guard destructs
    REQUIRE(lock.try_lock());
    lock.unlock();
}