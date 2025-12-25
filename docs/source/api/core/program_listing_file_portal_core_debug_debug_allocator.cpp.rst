
.. _program_listing_file_portal_core_debug_debug_allocator.cpp:

Program Listing for File debug_allocator.cpp
============================================

|exhale_lsh| :ref:`Return to documentation for file <file_portal_core_debug_debug_allocator.cpp>` (``portal\core\debug\debug_allocator.cpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   // //
   // // Copyright © 2025 Jonatan Nevo.
   // // Distributed under the MIT license (see LICENSE file).
   // //
   //
   // #include "debug_allocator.h"
   //
   // #include <atomic>
   // #include <iostream>
   // #include <cstdlib>
   //
   // #ifdef PORTAL_PLATFORM_WINDOWS
   // #include <windows.h>
   // #include <malloc.h>
   // #undef min
   // #undef max
   // #endif
   //
   // #include "portal/core/log.h"
   //
   //
   // namespace portal
   // {
   //
   // struct DebugAllocatorHeader
   // {
   //     size_t size{};
   //     uint32_t offset{};
   // };
   //
   // inline void report_allocator_error(const char* message)
   // {
   //     LOG_ERROR_TAG("DebugAllocator", message);
   // #ifdef PORTAL_PLATFORM_WINDOWS
   //     OutputDebugStringA(message);
   // #endif
   //
   //     throw std::bad_alloc();
   // }
   //
   // inline DebugAllocatorHeader* get_allocation_header(void* memory)
   // {
   //     return static_cast<DebugAllocatorHeader*>(memory) - 1;
   // }
   //
   // void* DebugAllocator::allocate(const size_t size)
   // {
   // #ifdef PORTAL_DEBUG_ALLOCATIONS
   //
   //     const size_t aligned_header_size = align(sizeof(DebugAllocatorHeader), alignment);
   //     const size_t allocation_size = size + alignment - 1 + aligned_header_size;
   //
   //     auto* memory = static_cast<uint8_t*>(std::malloc(allocation_size));
   //     PORTAL_TRACE_ALLOC(memory, allocation_size);
   //     if (memory == nullptr)
   //         return nullptr;
   //
   //     uint8_t* aligned_memory = align(memory, alignment) + aligned_header_size;
   //     DebugAllocatorHeader* header = get_allocation_header(memory);
   //     *header = {
   //         .size = allocation_size,
   //         .alignment = static_cast<uint32_t>(alignment),
   //         .offset = static_cast<uint32_t>(aligned_memory - memory)
   //     };
   //
   //     params.allocated_size.fetch_add(allocation_size, std::memory_order_relaxed);
   //     params.allocation_num.fetch_add(1, std::memory_order_relaxed);
   //     return aligned_memory;
   // #else
   // #ifdef PORTAL_COMPILER_MSVC
   //     void* ptr = _aligned_malloc(size, alignment);
   // #else
   //     void* ptr = std::aligned_alloc(size, alignment);
   // #endif
   //     PORTAL_TRACE_ALLOC(ptr, size);
   //     return ptr;
   // #endif
   // }
   //
   //
   // void DebugAllocator::deallocate(void* p) noexcept
   // {
   //     if (p == nullptr)
   //         return;
   //
   // #ifdef PORTAL_DEBUG_ALLOCATIONS
   //     const DebugAllocatorHeader* header = get_allocation_header(p);
   //     const size_t allocated_size = params.allocated_size.fetch_sub(header->size, std::memory_order_relaxed);
   //     const size_t allocation_num = params.allocation_num.fetch_sub(1, std::memory_order_relaxed);
   //
   //     if (allocated_size < header->size)
   //         report_allocator_error("debug_aligned_free() failed: memory allocation size is too small.");
   //     if (allocation_num == 0)
   //         report_allocator_error("debug_aligned_free() failed: no allocations left to free.");
   //     PORTAL_TRACE_FREE(static_cast<byte*>(p) - header->offset);
   //     std::free(static_cast<byte*>(p) - header->offset);
   // #else
   //     PORTAL_TRACE_FREE(p);
   // #ifdef PORTAL_COMPILER_MSVC
   //     _aligned_free(p);
   // #else
   //     std::free(p);
   // #endif
   // #endif
   // }
   // } // portal
   //
   // portal::DebugAllocator& get_allocator() noexcept
   //     {
   //         static portal::DebugAllocator allocator;
   //         return allocator;
   //     }
