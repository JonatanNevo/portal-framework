//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "portal/engine/allocators/pool_allocator.h"

struct A
{
    A()
    {
        std::printf("A constructed\n");
    }
    ~A()
    {
        std::printf("A destroyed\n");
    }
    uint64_t a;
};

int main()
{
    portal::PoolAllocator<A, 4> allocator;
    auto* a = allocator.alloc();
    auto* b = allocator.alloc();
    auto* c = allocator.alloc();
    auto* d = allocator.alloc();

    allocator.free(a);
    allocator.free(b);
    allocator.free(c);
    allocator.free(d);
}
