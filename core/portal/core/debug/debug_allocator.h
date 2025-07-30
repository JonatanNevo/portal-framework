//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <atomic>
#include "profile.h"

namespace portal
{

template <typename T, typename A>
constexpr T align(T x, A alignment)
{
    return (T)((size_t(x) + (size_t)alignment - 1) & ~((size_t)alignment - 1));
}

struct AllocationParams
{
    std::atomic<uint64_t> allocation_num = 0;
    std::atomic<size_t> allocated_size = 0;
};

class DebugAllocator
{
public:
    DebugAllocator() = default;
    virtual ~DebugAllocator() = default;

    [[nodiscard]] void* allocate(size_t size, size_t alignment);
    [[nodiscard]] void* reallocate(void* p, size_t size, size_t alignment);
    void deallocate(void* p) noexcept;

    AllocationParams params{};
};

/**
 * STD container wrapper for the debug allocator.
 */
template <typename T>
class StdDebugAllocator final : DebugAllocator
{
public:
    typedef T value_type;
    using is_always_equal = std::true_type;

    StdDebugAllocator() = default;

    template <class U>
    explicit constexpr StdDebugAllocator(const StdDebugAllocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(const size_t n)
    {
        return static_cast<T*>(DebugAllocator::allocate(n * sizeof(T), 1));
    }

    void deallocate(T* p, size_t) noexcept
    {
        DebugAllocator::deallocate(p);
    }

    bool operator==(const DebugAllocator& other) const noexcept
    {
        return &other == this;
    }
};


} // portal

// #if defined(PORTAL_DEBUG_ALLOCATIONS) || defined(PORTAL_PROFILE)
// inline portal::DebugAllocator& get_allocator() noexcept
// {
//     static portal::DebugAllocator allocator;
//     return allocator;
// }
//
// void* operator new(const size_t size)
// {
//     auto& allocator = get_allocator();
//     return allocator.allocate(size, 1);
// };
//
// void* operator new[](const size_t size)
// {
//     auto& allocator = get_allocator();
//     return allocator.allocate(size, 1);
// }
//
// void operator delete(void* block) noexcept
// {
//     auto& allocator = get_allocator();
//     allocator.deallocate(block);
// }
//
// void operator delete[](void* block) noexcept
// {
//     auto& allocator = get_allocator();
//     allocator.deallocate(block);
// }
// #endif

