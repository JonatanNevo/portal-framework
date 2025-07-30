//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "debug_allocator.h"

#include <atomic>
#include <iostream>
#include <cstdlib>

#ifdef PORTAL_PLATFORM_WINDOWS
#include <windows.h>
#include <malloc.h>
#undef min
#undef max
#endif

#include "portal/core/log.h"


namespace portal
{

struct DebugAllocatorHeader
{
    size_t size{};
    uint32_t alignment{};
    uint32_t offset{};
};

inline void report_allocator_error(const char* message)
{
    LOG_ERROR_TAG("DebugAllocator", message);
#ifdef PORTAL_PLATFORM_WINDOWS
    OutputDebugStringA(message);
#endif

    throw std::bad_alloc();
}

inline DebugAllocatorHeader* get_allocation_header(void* memory)
{
    return static_cast<DebugAllocatorHeader*>(memory) - 1;
}

void* DebugAllocator::allocate(const size_t size, const size_t alignment)
{
#ifdef PORTAL_DEBUG_ALLOCATIONS
    if (alignment == 0)
        report_allocator_error("alignment cant be 0");

    const size_t aligned_header_size = align(sizeof(DebugAllocatorHeader), alignment);
    const size_t allocation_size = size + alignment - 1 + aligned_header_size;

    auto* memory = static_cast<uint8_t*>(std::malloc(allocation_size));
    PORTAL_TRACE_ALLOC(memory, allocation_size);
    if (memory == nullptr)
        return nullptr;

    uint8_t* aligned_memory = align(memory, alignment) + aligned_header_size;
    DebugAllocatorHeader* header = get_allocation_header(memory);
    *header = {
        .size = allocation_size,
        .alignment = static_cast<uint32_t>(alignment),
        .offset = static_cast<uint32_t>(aligned_memory - memory)
    };

    params.allocated_size.fetch_add(allocation_size, std::memory_order_relaxed);
    params.allocation_num.fetch_add(1, std::memory_order_relaxed);
    return aligned_memory;
#else
#ifdef PORTAL_PLATFORM_WINDOWS
    void* ptr = _aligned_malloc(size, alignment);
#else
    void* ptr = std::aligned_alloc(size, alignment);
#endif
    PORTAL_TRACE_ALLOC(ptr, size);
    return ptr;
#endif
}

void* DebugAllocator::reallocate(void* p, const size_t size, size_t alignment)
{
#ifdef PORTAL_DEBUG_ALLOCATIONS
    if (alignment == 0)
        report_allocator_error("alignment can't be 0.");

    if (p == nullptr)
        return allocate(size, alignment);

    const DebugAllocatorHeader prev_header = *get_allocation_header(p);

    if (prev_header.alignment != alignment)
        report_allocator_error("DebugAlignedRealloc() failed: memory alignment mismatch.");

    const size_t aligned_header_size = align(sizeof(DebugAllocatorHeader), alignment);
    const size_t allocation_size = size + alignment - 1 + aligned_header_size;

    uint8_t* prev_memory_begin = static_cast<uint8_t*>(p) - prev_header.offset;
    const auto new_memory = static_cast<uint8_t*>(std::realloc(prev_memory_begin, allocation_size));
    PORTAL_TRACE_REALLOC(p, new_memory, allocation_size);

    if (new_memory == nullptr)
        return nullptr;

    uint8_t* aligned_memory = align(new_memory, alignment) + aligned_header_size;

    DebugAllocatorHeader* new_header = get_allocation_header(aligned_memory);
    *new_header = {
        .size = allocation_size,
        .alignment = static_cast<uint32_t>(alignment),
        .offset = static_cast<uint32_t>(aligned_memory - new_memory)
    };

    params.allocated_size.fetch_add(allocation_size - prev_header.size, std::memory_order_relaxed);
    return aligned_memory;
#else
    if (size == 0) {
        PORTAL_TRACE_FREE(p);
        std::free(p);
        return nullptr;
    }

#ifdef PORTAL_PLATFORM_WINDOWS
    void* new_ptr = _aligned_malloc(size, alignment);
#else
    void* new_ptr = std::aligned_alloc(size, alignment);
#endif
    if (new_ptr == nullptr) {
        return nullptr;
    }

    PORTAL_TRACE_ALLOC(new_ptr, size);


    if (p != nullptr) {
        std::memcpy(new_ptr, p, size);
        PORTAL_TRACE_FREE(p);
#ifdef PORTAL_PLATFORM_WINDOWS
        _aligned_free(p);
#else
        std::free(p);
#endif
    }
    return new_ptr;
#endif
}

void DebugAllocator::deallocate(void* p) noexcept
{
    if (p == nullptr)
        return;

#ifdef PORTAL_DEBUG_ALLOCATIONS
    const DebugAllocatorHeader* header = get_allocation_header(p);
    const size_t allocated_size = params.allocated_size.fetch_sub(header->size, std::memory_order_relaxed);
    const size_t allocation_num = params.allocation_num.fetch_sub(1, std::memory_order_relaxed);

    if (allocated_size < header->size)
        report_allocator_error("debug_aligned_free() failed: memory allocation size is too small.");
    if (allocation_num == 0)
        report_allocator_error("debug_aligned_free() failed: no allocations left to free.");
    PORTAL_TRACE_FREE(static_cast<byte*>(p) - header->offset);
    std::free(static_cast<byte*>(p) - header->offset);
#else
    PORTAL_TRACE_FREE(p);
#ifdef PORTAL_PLATFORM_WINDOWS
    _aligned_free(p);
#else
    std::free(p);
#endif
#endif
}
} // portal
