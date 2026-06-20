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

TEST_CASE("SpinLock provides mutual exclusion", "[spinlock]")
{
    SpinLock lock;

    // overlap detector: must never see another thread inside the critical section
    std::atomic<int> inside{0};
    std::atomic<bool> overlap_detected{false};

    // lost-update detector: non-atomic counter only safe under correct exclusion
    int counter = 0;

    const unsigned hw = std::max(2u, std::thread::hardware_concurrency());
    const unsigned thread_count = hw * 2;
    constexpr int iterations = 100'000;

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (unsigned t = 0; t < thread_count; ++t)
    {
        threads.emplace_back(
            [&]
            {
                for (int i = 0; i < iterations; ++i)
                {
                    std::lock_guard guard(lock);

                    if (inside.fetch_add(1, std::memory_order_relaxed) != 0)
                        overlap_detected.store(true, std::memory_order_relaxed);

                    // widen the window so a broken lock is likely to overlap
                    volatile int sink = 0;
                    for (int spin = 0; spin < 4; ++spin)
                        sink = sink + 1;
                    ++counter;

                    inside.fetch_sub(1, std::memory_order_relaxed);
                }
            });
    }

    for (auto& thread : threads)
        thread.join();

    REQUIRE_FALSE(overlap_detected.load(std::memory_order_relaxed));
    REQUIRE(counter == static_cast<int>(thread_count) * iterations);
}