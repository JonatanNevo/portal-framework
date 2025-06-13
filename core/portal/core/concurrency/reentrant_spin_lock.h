//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <atomic>
#include <thread>

#include "portal/core/assert.h"

namespace portal
{
/**
 * A performant reentrant spin lock, for use in places where each thread is expected to hold the lock for a very short time.
 * This lock meets the `BasicLockable` and `Lockable` requirements, and can be used with `std::lock_guard`, `std::scoped_lock` or `std::unique_lock`.
 *
 * This class is templated to allow for different ref count sizes.
 *
 * This is based on the spinlock implementation from the book "Game Engine Architecture" by Jason Gregory.
 */
template <typename T = uint32_t> requires std::is_integral_v<T>
class ReentrantSpinLock
{
public:
    ReentrantSpinLock(): locked_thread(0), ref_count(0) {}

    /**
    * Attempts to acquire the lock for the current thread without blocking.
    *
    * @return `true` if the lock was acquired, `false` otherwise.
    */
    bool try_lock()
    {
        constexpr std::hash<std::thread::id> hasher;
        const size_t thread_id = hasher(std::this_thread::get_id());

        bool acquired = false;

        if (locked_thread.load(std::memory_order_relaxed) == thread_id)
        {
            acquired = true;
        }
        else
        {
            size_t unlocked_value = 0;
            acquired = locked_thread.compare_exchange_strong(unlocked_value, thread_id, std::memory_order_relaxed, std::memory_order_relaxed);
        }

        if (acquired)
        {
            ++ref_count;
            std::atomic_thread_fence(std::memory_order_acquire);
        }

        return acquired;
    }

    /**
     * Blocks until the lock can be acquired for the current thread.
     */
    void lock()
    {
        constexpr std::hash<std::thread::id> hasher;
        const size_t thread_id = hasher(std::this_thread::get_id());

        // If this thread does not already hold the lock
        if (locked_thread.load(std::memory_order_relaxed) != thread_id)
        {
            // spin wait until we can acquire the lock
            size_t unlocked_value = 0;
            while (!locked_thread.compare_exchange_weak(unlocked_value, thread_id, std::memory_order_relaxed, std::memory_order_relaxed))
            {
                unlocked_value = 0;
                std::this_thread::yield();
            }
        }

        // Increment the reference count for this thread, then fence to ensure all subsequent reads by this thread will be visible
        ++ref_count;
        std::atomic_thread_fence(std::memory_order_acquire);
    }

    /**
    * Releases the non-shared lock held by the thread.
    *
    * The calling thread must have previously acquired the lock.
    */
    void unlock()
    {
        // Use release semantics to ensure that all prior writes have been fully committed before we unlock
        std::atomic_thread_fence(std::memory_order_release);

        constexpr std::hash<std::thread::id> hasher;
        [[maybe_unused]] const size_t thread_id = hasher(std::this_thread::get_id());
        [[maybe_unused]] const size_t actual_value = locked_thread.load(std::memory_order_relaxed);

        PORTAL_ASSERT(actual_value == thread_id, "Unlocking a reentrant spin lock that is not held by the current thread");

        --ref_count;
        if (ref_count == 0)
        {
            // release lock, which is safe because we are owning it.
            locked_thread.store(0, std::memory_order_relaxed);
        }
    }

private:
    std::atomic<size_t> locked_thread;
    T ref_count;
};
}
