//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <atomic>
#include <thread>

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
public:
    SpinLock() = default;

    /**
     * Attempts to acquire the lock for the current thread without blocking.
     *
     * @return `true` if the lock was acquired, `false` otherwise.
     */
    bool try_lock()
    {
        // Use an acquire fence to ensure all subsequent reads by this thread will be valid
        const bool already_locked = locked.test_and_set(std::memory_order_acquire);
        return !already_locked;
    }

    /**
     * Blocks until the lock can be acquired for the current thread.
     */
    void lock()
    {
        unsigned backoff = 1;

        if (try_lock())
            return;

        // Exponential back-off strategy
        while (true)
        {
            for (unsigned i = 0; i < backoff; ++i)
                std::this_thread::yield();

            if (try_lock())
                return;

            // Increase back-off time (with a reasonable upper limit)
            if (backoff < 1024)
                backoff *= 2;
        }
    }

    /**
     * Releases the non-shared lock held by the thread.
     *
     * The calling thread must have previously acquired the lock.
     */
    void unlock() noexcept
    {
        // Use release semantics to ensure that all prior write have been fully commited before we unlock
        locked.clear(std::memory_order_release);
    }

private:
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
};
}
