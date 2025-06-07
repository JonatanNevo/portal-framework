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
 * A performant spin lock, for use in places where each thread is expected to hold the lock for a very short time.
 * This lock meets the `BasicLockable` and `Lockable` requirements, and can be used with `std::lock_guard`, `std::scoped_lock` or `std::unique_lock`.
 *
 * This is based on the spinlock implementation from the book "Game Engine Architecture" by Jason Gregory.
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
        // Spin until we can acquire the lock
        while (!try_lock())
        {
            // Reduce power consumption by yielding the thread
            std::this_thread::yield();
        }
    }

    /**
     * Releases the non-shared lock held by the thread.
     *
     * The calling thread must have previously acquired the lock.
     */
    void unlock()
    {
        // Use release semantics to ensure that all prior write have been fully commited before we unlock
        locked.clear(std::memory_order_release);
    }

private:
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
};

}
