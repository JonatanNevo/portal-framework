//
// Created by thejo on 4/23/2025.
//

#pragma once
#include "portal/core/assert.h"

namespace portal
{

namespace internal
{
    static void* (*alloc)(size_t size) = [](const size_t size) { return std::malloc(size); };
    static void (*release)(void* ptr) = [](void* ptr) { free(ptr); };

    using AllocateCallback = void* (*)(size_t size);
    using FreeCallback = void(*)(void* ptr);

    inline void set_allocation_callbacks(AllocateCallback allocate_callback, FreeCallback free_callback)
    {
        alloc = allocate_callback;
        release = free_callback;
    }
}

template <size_t MaxStackSize>
class InlineAllocator
{
public:
    constexpr InlineAllocator() noexcept :
        size(0)
    {
        PORTAL_CORE_ASSERT(
            MaxStackSize > sizeof(void*),
            "MaxStackSize is smaller or equal to the size of a pointer. This will make the use of an InlineAllocator pointless. Please increase the MaxStackSize."
            );
    }

    ~InlineAllocator() noexcept
    {
        release();
    }

    InlineAllocator(const InlineAllocator& other) :
        size(0)
    {
        if (other.has_allocation())
        {
            memcpy(allocate(other.size), other.get_allocation(), other.size);
        }
        size = other.size;
    }

    InlineAllocator& operator=(const InlineAllocator& other)
    {
        if (other.has_allocation())
        {
            memcpy(allocate(other.size), other.get_allocation(), other.size);
        }
        size = other.size;
        return *this;
    }

    InlineAllocator(InlineAllocator&& other) noexcept :
        size(other.size)
    {
        other.size = 0;
        if (size > MaxStackSize)
        {
            std::swap(ptr, other.ptr);
        }
        else
        {
            memcpy(buffer, other.buffer, size);
        }
    }

    InlineAllocator& operator=(InlineAllocator&& other) noexcept
    {
        release();
        size = other.size;
        other.size = 0;
        if (size > MaxStackSize)
        {
            std::swap(ptr, other.ptr);
        }
        else
        {
            memcpy(buffer, other.buffer, size);
        }
        return *this;
    }

    /**
     * Allocates memory of a given size
     */
    void* allocate(const size_t alloc_size)
    {
        if (this->size != alloc_size)
        {
            release();
            this->size = alloc_size;

            if (alloc_size > MaxStackSize)
            {
                ptr = internal::alloc(alloc_size);
                return ptr;
            }
        }
        return reinterpret_cast<void*>(buffer);
    }

    void release()
    {
        if (size > MaxStackSize)
            internal::release(ptr);
        size = 0;
    }

    [[nodiscard]] void* get_allocation() const
    {
        if (has_allocation())
        {
            if (has_heap_allocation())
                return ptr;
            return const_cast<char*>(buffer);
        }
        return nullptr;
    }

    [[nodiscard]] size_t get_size() const
    {
        return size;
    }

    [[nodiscard]] bool has_allocation() const
    {
        return size > 0;
    }

    [[nodiscard]] bool has_heap_allocation() const
    {
        return size > MaxStackSize;
    }

private:
    // If the allocation is smaller than the threshold, Buffer is used
    // Otherwise ptr is used together with a separate dynamic allocation
    union
    {
        char buffer[MaxStackSize];
        void* ptr;
    };

    size_t size;
};
}
