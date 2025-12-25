
.. _program_listing_file_portal_core_debug_debug_allocator.h:

Program Listing for File debug_allocator.h
==========================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_debug_debug_allocator.h>` (``portal\core\debug\debug_allocator.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   // //
   // // Copyright © 2025 Jonatan Nevo.
   // // Distributed under the MIT license (see LICENSE file).
   // //
   //
   // #pragma once
   // #include <atomic>
   // #include "profile.h"
   //
   // namespace portal
   // {
   // #define PORTAL_DEBUG_ALLOCATIONS
   //
   // struct AllocationParams
   // {
   //     std::atomic<uint64_t> allocation_num = 0;
   //     std::atomic<size_t> allocated_size = 0;
   // };
   //
   // class DebugAllocator
   // {
   // public:
   //     DebugAllocator() = default;
   //     virtual ~DebugAllocator() = default;
   //
   //     [[nodiscard]] void* alloc(size_t size);
   //     void free(void* p) noexcept;
   //
   //     AllocationParams params{};
   // };
   //
   // /**
   //  * STD container wrapper for the debug allocator.
   //  */
   // template <typename T>
   // class StdDebugAllocator final : DebugAllocator
   // {
   // public:
   //     typedef T value_type;
   //     using is_always_equal = std::true_type;
   //
   //     StdDebugAllocator() = default;
   //
   //     template <class U>
   //     explicit constexpr StdDebugAllocator(const StdDebugAllocator<U>&) noexcept {}
   //
   //     [[nodiscard]] T* allocate(const size_t n)
   //     {
   //         return static_cast<T*>(alloc(n * sizeof(T)));
   //     }
   //
   //     void deallocate(T* p, size_t) noexcept
   //     {
   //         DebugAllocator::free(p);
   //     }
   //
   //     bool operator==(const DebugAllocator& other) const noexcept
   //     {
   //         return &other == this;
   //     }
   // };
   //
   //
   // } // portal
   //
   // #if defined(PORTAL_DEBUG_ALLOCATIONS) || defined(PORTAL_PROFILE)
   // portal::DebugAllocator& get_allocator() noexcept;
   //
   // inline void* operator new(const size_t size)
   // {
   //     auto& allocator = get_allocator();
   //     return allocator.alloc(size);
   // };
   //
   // inline void* operator new[](const size_t size)
   // {
   //     auto& allocator = get_allocator();
   //     return allocator.alloc(size);
   // }
   //
   // inline void operator delete(void* block) noexcept
   // {
   //     auto& allocator = get_allocator();
   //     allocator.free(block);
   // }
   //
   // inline void operator delete[](void* block) noexcept
   // {
   //     auto& allocator = get_allocator();
   //     allocator.free(block);
   // }
   // #endif
   //
   //
