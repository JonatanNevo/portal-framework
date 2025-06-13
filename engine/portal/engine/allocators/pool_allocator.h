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
    PoolAllocator() noexcept { clear(); }

    /**
     * Allocates and constructs a new object of type T
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Pointer to the new object
     */
    template<typename... Args>
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
    constexpr static auto pool_size = C * sizeof(T);
    std::array<uint8_t, pool_size> pool{};
    void** head = nullptr;
    bool full = false;
    L lock_object{};
};

} // portal
