//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <array>
#include <mutex>
#include <new>

#include "portal/core/concurrency/spin_lock.h"

namespace portal
{
/**
 * Fixed-size object pool with O(1) allocation/deallocation using embedded freelist.
 *
 * PoolAllocator pre-allocates a fixed buffer for C instances of type T and manages
 * them using a freelist embedded directly in the free blocks. This makes allocation
 * and deallocation O(1) pointer operations with LIFO reuse (last freed is first
 * allocated), which tends to have good cache characteristics.
 *
 * The freelist works by storing pointers to the next free block inside each free
 * block itself. This is why the constraint `sizeof(T) >= sizeof(void*)` exists:
 * each free block must be large enough to hold a pointer to the next free block.
 * Types smaller than a pointer (like bool, char, uint16_t on 64-bit systems) cannot
 * be pooled with this allocator.
 *
 * Thread Safety: Controlled by template parameter L. By default uses SpinLock,
 * which is ideal for very short-lived locks (microseconds) with rare contention.
 * For longer critical sections or higher contention, substitute std::mutex:
 * `PoolAllocator<MyType, 1000, std::mutex>`.
 *
 * Example - Particle system with object pooling:
 * @code
 * PoolAllocator<Particle, 1000> particle_pool;
 *
 * void spawn_particle(Vector3 pos, Vector3 vel) {
 *     auto* particle = particle_pool.alloc(pos, vel);  // O(1) allocation
 *     active_particles.push_back(particle);
 * }
 *
 * void update_particles(float dt) {
 *     for (auto it = active_particles.begin(); it != active_particles.end();) {
 *         (*it)->update(dt);
 *         if ((*it)->is_dead()) {
 *             particle_pool.free(*it);  // O(1) deallocation, returns to freelist
 *             it = active_particles.erase(it);
 *         } else {
 *             ++it;
 *         }
 *     }
 * }
 * @endcode
 *
 * Important Notes:
 * - Allocation throws std::bad_alloc when pool is exhausted (all C slots used)
 * - clear() rebuilds freelist WITHOUT calling destructors - only use when pool
 *   is empty or all objects are trivially destructible
 * - Freelist reuse is LIFO order (last freed, first allocated)
 * - Pool memory is never deallocated until PoolAllocator is destroyed
 *
 * @tparam T The object type to pool. Must satisfy `sizeof(T) >= sizeof(void*)`
 *           because free blocks embed next-pointers in themselves. Common pooled
 *           types: game entities, particles, messages, temporary objects.
 * @tparam C Pool capacity (maximum number of T instances). Pool size is C * sizeof(T).
 *           Set based on worst-case usage to avoid exhaustion.
 * @tparam L Lock type for thread safety. Defaults to SpinLock. Use std::mutex for
 *           longer critical sections or high contention. Use a no-op lock type for
 *           single-threaded scenarios to eliminate locking overhead.
 *
 * @see BucketPoolAllocator for variable-sized allocations up to a bucket size
 * @see SpinLock for the default lock implementation
 */
template <typename T, size_t C, typename L=SpinLock> requires (sizeof(T) >= sizeof(void*))
class PoolAllocator
{
public:
    /**
     * Total size of the pool in bytes (C instances * sizeof(T)).
     */
    constexpr static auto pool_size = C * sizeof(T);

    PoolAllocator() noexcept { clear(); }

    /**
     * Allocates and constructs a new object of type T
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Pointer to the new object
     */
    template <typename... Args>
    T* alloc(Args&&... args)
    {
        std::lock_guard<L> lock(lock_object);
        if (full)
            throw std::bad_alloc();

        auto allocated_mem = reinterpret_cast<T*>(head);
        head = static_cast<void**>(*head);
        if (reinterpret_cast<void*>(head) == pool.data() + pool_size)
            full = true;

        return new(allocated_mem) T(std::forward<Args>(args)...);
    }

    /**
     * Frees the specified allocated pointer
     * @param p The pointer to free
     */
    void free(T* p)
    {
        std::lock_guard<L> lock(lock_object);
        if (p == nullptr)
            return;

        p->~T();

        if (full)
            full = false;

        // TODO: Iterate over tree?
        *reinterpret_cast<void**>(p) = head;
        head = reinterpret_cast<void**>(p);
    }

    /**
     * Rebuilds the freelist, making all pool slots available again.
     *
     * WARNING: This does NOT call destructors on allocated objects. Calling
     * clear() with active allocations causes RESOURCE LEAKS and UNDEFINED BEHAVIOR
     * for types managing resources (heap memory, file handles, mutexes, etc.).
     * This is a CORRECTNESS and SAFETY issue, not merely a performance concern.
     *
     * Only use clear() when:
     * - The pool is completely empty (all objects freed), OR
     * - All allocated objects are trivially destructible (no resources to clean up)
     *
     * For proper cleanup, call free() on ALL allocated objects before clear().
     */
    void clear()
    {
        std::lock_guard<L> lock(lock_object);
        // Initialize the pool with pointers to the next free block (as offsets)
        for (size_t i = 0; i < pool_size; i += sizeof(T))
        {
            if (i + sizeof(T) == pool_size)
                *reinterpret_cast<void**>(pool.data() + i) = nullptr;
            *reinterpret_cast<void**>(pool.data() + i) = reinterpret_cast<void*>(pool.data() + (i + sizeof(T)));
        }
        head = reinterpret_cast<void**>(pool.data());
        full = false;
    }

private:
    std::array<uint8_t, pool_size> pool{};
    void** head = nullptr;
    bool full = false;
    L lock_object{};
};

/**
 * Variable-size pool allocator with fixed-size buckets and embedded freelist.
 *
 * BucketPoolAllocator is a variant of PoolAllocator for raw memory allocations
 * (void*) up to a maximum bucket size. Instead of typed objects, it allocates
 * fixed-size buckets that can hold variable-sized data. This is useful for
 * small message queues, string buffers, or other variable-sized temporary
 * allocations where the maximum size is known.
 *
 * Like PoolAllocator, this uses an embedded freelist for O(1) allocation and
 * deallocation, with the same LIFO reuse pattern. The constraint
 * `B >= sizeof(void*)` exists for the same reason: free buckets must be large
 * enough to store a pointer to the next free bucket.
 *
 * The optional `check_allocations` template parameter enables allocation
 * tracking using an atomic counter. When enabled, get_allocation_size()
 * returns the number of currently allocated buckets, useful for debugging
 * or telemetry.
 *
 * Example - Small message queue:
 * @code
 * // Pool for messages up to 256 bytes, max 100 messages
 * BucketPoolAllocator<256, 100> message_pool;
 *
 * void send_message(const void* data, size_t size) {
 *     if (size > 256) {
 *         // Message too large, handle error
 *         return;
 *     }
 *     void* bucket = message_pool.alloc();
 *     std::memcpy(bucket, data, size);
 *     message_queue.push({bucket, size});
 * }
 *
 * void process_messages() {
 *     while (!message_queue.empty()) {
 *         auto [bucket, size] = message_queue.pop();
 *         handle_message(bucket, size);
 *         message_pool.free(bucket);
 *     }
 * }
 * @endcode
 *
 * @tparam B Bucket size in bytes. All allocations return B-byte buckets regardless
 *           of requested size. Must be >= sizeof(void*) for freelist embedding.
 *           Choose based on maximum expected allocation size.
 * @tparam C Pool capacity (maximum number of buckets). Total pool size is B * C.
 * @tparam L Lock type for thread safety. Defaults to SpinLock. Same semantics as
 *           PoolAllocator - use std::mutex for high contention.
 * @tparam check_allocations If true, enables allocation tracking with atomic counter.
 *           Adds small overhead but allows querying allocated bucket count via
 *           get_allocation_size(). Defaults to false.
 *
 * @see PoolAllocator for typed object pools
 * @see SpinLock for the default lock implementation
 */
template <size_t B, size_t C, typename L=SpinLock, bool check_allocations = false> requires (B >= sizeof(void*))
class BucketPoolAllocator
{
public:
    /**
     * Size of each bucket in bytes.
     */
    constexpr static auto bucket_size = B;
    /**
     * Total size of the pool in bytes (C buckets * B bytes).
     */
    constexpr static auto pool_size = C * B;

    BucketPoolAllocator() noexcept { clear(); }

    /**
     * Allocates a fixed-size bucket from the pool.
     *
     * Returns a B-byte bucket regardless of how much space you actually need.
     * The caller is responsible for not writing beyond their actual data size.
     *
     * @return Pointer to a B-byte bucket
     * @throws std::bad_alloc if pool is exhausted (all C buckets allocated)
     */
    void* alloc()
    {
        std::lock_guard<L> lock(lock_object);
        if (full)
            throw std::bad_alloc();

        const auto allocated_mem = reinterpret_cast<void*>(head);
        head = static_cast<void**>(*head);
        if (reinterpret_cast<void*>(head) == pool.data() + pool_size)
            full = true;

        if constexpr (check_allocations)
            ++allocated_buckets;

        return allocated_mem;
    }

    /**
     * Frees the specified allocated pointer
     * @param p The pointer to free
     */
    void free(void* p)
    {
        std::lock_guard<L> lock(lock_object);
        if (p == nullptr)
            return;

        if (full)
            full = false;

        // TODO: Iterate over tree?
        *static_cast<void**>(p) = head;
        head = static_cast<void**>(p);

        if constexpr (check_allocations)
            --allocated_buckets;
    }

    /**
     * Clears the entire pool allocator
     */
    void clear()
    {
        std::lock_guard<L> lock(lock_object);
        // Initialize the pool with pointers to the next free block (as offsets)
        for (size_t i = 0; i < pool_size; i += bucket_size)
        {
            if (i + bucket_size == pool_size)
                *reinterpret_cast<void**>(pool.data() + i) = nullptr;
            *reinterpret_cast<void**>(pool.data() + i) = reinterpret_cast<void*>(pool.data() + (i + bucket_size));
        }
        head = reinterpret_cast<void**>(pool.data());
        full = false;
    }

    /**
     * Returns the number of currently allocated buckets.
     *
     * Only available when check_allocations template parameter is true.
     * Uses atomic operations for thread-safe reading.
     *
     * @return Number of buckets currently allocated (not freed)
     */
    [[nodiscard]] size_t get_allocation_size() const requires (check_allocations)
    {
        return allocated_buckets.load();
    }

private:
    std::array<uint8_t, pool_size> pool{};
    std::atomic<size_t> allocated_buckets = 0;

    void** head = nullptr;
    bool full = false;
    L lock_object{};
};
} // portal
