//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "stack_allocator.h"

#include <stdexcept>

namespace portal
{

constexpr size_t DEFAULT_SIZE = 1024;

SStackAllocator::SStackAllocator(): SStackAllocator(DEFAULT_SIZE)
{}

SStackAllocator::SStackAllocator(const size_t total_size) :
    buffer(total_size), top(0) {}

void* SStackAllocator::alloc(const size_t size)
{
    if (top + size > buffer.size()) {
        throw std::bad_alloc(); // Not enough space
    }

    void* p = buffer.data() + top;
    allocations[p] = size;
    top += size;

    return p;
}

void SStackAllocator::free(void* p)
{
    if (!allocations.contains(p)) {
        throw std::invalid_argument("Pointer not allocated by this stack allocator");
    }

    const size_t size = allocations[p];
    allocations.erase(p);
    top -= size;
}

SStackAllocator::marker SStackAllocator::get_marker() const
{
    return top;
}

size_t SStackAllocator::get_size() const {
    return buffer.size();
}

void SStackAllocator::free_to_marker(const marker m)
{
    // Ignores the allocations map, worst case we will override it.
    top = m;
}

void SStackAllocator::clear()
{
    top = 0;
    allocations.clear();
}

void SStackAllocator::resize(const size_t new_size)
{
    clear();
    buffer.resize(new_size);
}
} // portal
