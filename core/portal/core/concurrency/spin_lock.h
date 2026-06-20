//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <atomic>
#include <thread>

#include "cpu_relax.h"

namespace portal
{
/**
 * High-performance spin lock for very short critical sections (microseconds).
 *
 * Busy-waits instead of sleeping when contended, making it faster than std::mutex for
 * brief critical sections but wasteful for longer ones. Uses exponential backoff (1-1024
 * yields) to reduce CPU waste under contention.
 *
 * Use for: O(1) operations, rare contention, predictable short durations
 * Use std::mutex for: Longer sections, unpredictable duration, high contention
 *
 * Example:
 * @code
 * SpinLock stats_lock;
 * std::lock_guard lock(stats_lock);
 * stats.count++;  // Brief critical section
 * @endcode
 *
 * @note Not reentrant - same thread locking twice deadlocks (use ReentrantSpinLock)
 * @see ReentrantSpinLock for reentrant variant
 */
class SpinLock
{
    static constexpr auto SPIN_BACKOFF_COUNT = 16;

public:
    SpinLock() = default;

    /**
     * Attempts to acquire the lock for the current thread without blocking.
     *
     * @return `true` if the lock was acquired, `false` otherwise.
     */
    bool try_lock()
    {
        return !locked.load(std::memory_order_relaxed) && !locked.exchange(true, std::memory_order_acquire);
    }

    /**
     * Blocks until the lock can be acquired for the current thread.
     */
    void lock()
    {
        for (int spin_count = 0; !try_lock(); ++spin_count)
        {
            if (spin_count < SPIN_BACKOFF_COUNT)
            {
                cpu_relax();
            }
            else
            {
                std::this_thread::yield();
                spin_count = 0;
            }
        }
    }

    /**
     * Releases the non-shared lock held by the thread.
     *
     * The calling thread must have previously acquired the lock.
     */
    void unlock() noexcept
    {
        locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked = false;
};
}
