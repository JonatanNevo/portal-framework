//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "portal/core/concurrency/reentrant_spin_lock.h"

using namespace portal;

TEST_CASE("ReentrantSpinLock single lock/unlock", "[reentrant_spinlock]")
{
    ReentrantSpinLock<> lock;
    REQUIRE(lock.try_lock());
    lock.unlock();
    // free again after balanced unlock
    REQUIRE(lock.try_lock());
    lock.unlock();
}

TEST_CASE("ReentrantSpinLock is reentrant for the owning thread", "[reentrant_spinlock]")
{
    ReentrantSpinLock<> lock;

    SECTION("owner try_lock succeeds while already held")
    {
        REQUIRE(lock.try_lock());
        REQUIRE(lock.try_lock()); // same thread re-acquires
        lock.unlock();
        lock.unlock();
    }

    SECTION("lock releases only when ref count reaches zero")
    {
        constexpr int depth = 5;
        for (int i = 0; i < depth; ++i)
            lock.lock();

        // unbalance by one: still held, so another thread must fail to acquire
        for (int i = 0; i < depth - 1; ++i)
            lock.unlock();

        bool acquired_while_held = false;
        std::thread probe1([&] { acquired_while_held = lock.try_lock(); });
        probe1.join();
        REQUIRE_FALSE(acquired_while_held);

        // final unlock releases it
        lock.unlock();

        bool acquired_after_release = false;
        std::thread probe2(
            [&]
            {
                acquired_after_release = lock.try_lock();
                if (acquired_after_release)
                    lock.unlock();
            });
        probe2.join();
        REQUIRE(acquired_after_release);
    }
}

TEMPLATE_TEST_CASE("ReentrantSpinLock supports narrow ref-count types", "[reentrant_spinlock]", uint8_t, uint16_t, uint32_t)
{
    ReentrantSpinLock<TestType> lock;

    constexpr int depth = 10; // within range of all tested widths
    for (int i = 0; i < depth; ++i)
        REQUIRE(lock.try_lock());

    for (int i = 0; i < depth; ++i)
        lock.unlock();

    // fully released
    REQUIRE(lock.try_lock());
    lock.unlock();
}