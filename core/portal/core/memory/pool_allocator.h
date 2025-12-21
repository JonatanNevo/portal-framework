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
 * A pool allocator that allows quick allocations of multiple same size objects
 * @tparam T The object to pool
 * @tparam C The max number of objects that can be allocated (pool size is `sizeof(T) * C`)
 */
template <typename T, size_t C, typename L=SpinLock> requires (sizeof(T) >= sizeof(void*))
class PoolAllocator
{
public:
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
     * Clears the entire pool allocator
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
 * Same as pool allocator but instead of allocating and initializing a class, allocating a bucket of memory
 * @tparam B The bucket size
 * @tparam C The max number of buckets that can be allocated (pool size is `sizeof(T) * C`)
 */
template <size_t B, size_t C, typename L=SpinLock, bool check_allocations = false> requires (B >= sizeof(void*))
class BucketPoolAllocator
{
public:
    constexpr static auto bucket_size = B;
    constexpr static auto pool_size = C * B;

    BucketPoolAllocator() noexcept { clear(); }

    /**
     * Allocates a bucket in the pool
     * @return Pointer to the bucket
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
