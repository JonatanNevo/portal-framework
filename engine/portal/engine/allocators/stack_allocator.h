//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <array>

namespace portal
{

/**
 * A stack allocator that allows for allocations in a stack-like manner.
 */
class StackAllocator
{
public:
    using marker = size_t;

    /**
     * Constructs the stack allocator with default size
     */
    StackAllocator();

    /**
     * Constructs the stack allocator with the specified total size.
     *
     * @param total_size Total size of the stack allocator in bytes.
     */
    explicit StackAllocator(size_t total_size);

    /**
     * Allocates a given size from the top of the stack
     *
     * @param size The size to allocate
     * @return A pointer to the beginning of the allocated memory
     */
    void* alloc(size_t size);

    /**
     * Allocates memory and constructs an object of type T
     *
     * @tparam T The type of object to allocate
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Pointer to the new object
     */
    template <typename T, typename... Args>
    T* alloc(Args&&... args)
    {
        void* mem = alloc(sizeof(T));
        return new(mem) T(std::forward<Args>(args)...);
    }

    /**
     * Destroys and frees an object of type T
     *
     * @tparam T The type of object to free
     * @param p Pointer to the object
     */
    template <typename T>
    void free(T* p)
    {
        if (p == nullptr)
            return;

        // Call the destructor
        p->~T();

        // Free the memory
        this->free(reinterpret_cast<void*>(p));
    }

    /**
     * Frees an allocation made by this stack allocator.
     *
     * @param p The pointer to the memory to free. This must be a pointer allocated by this stack allocator.
     */
    void free(void* p);

    /**
     * @return A marker to the current top of the stack.
     */
    [[nodiscard]] marker get_marker() const;

    /**
     * Gets the total size of the stack allocator.
     *
     * @return The size of the stack in bytes.
     */
    [[nodiscard]] size_t get_size() const;

    /**
     * Frees the stack up to a given marker.
     * @param m the marker to free to
     */
    void free_to_marker(marker m);

    /**
     * Clears the entire stack
     */
    void clear();

    /** Resizes the stack allocator to a new size.
     * This will clear the current allocations and reset the stack.
     * @param new_size The new size of the stack in bytes.
     */
    void resize(size_t new_size);

private:
    std::vector<uint8_t> buffer;
    marker top;
    std::unordered_map<void*, size_t> allocations;

};


/**
 * A multi-buffered allocator that allows for multiple stack allocators to be used in a round-robin fashion.
 * @tparam N The number of buffers to use. Must be at least 2.
 */
template <unsigned int N> requires (N >= 2)
class BufferedAllocator
{
public:
    BufferedAllocator() = default;

    explicit BufferedAllocator(const size_t buffer_size)
    {
        for (auto& allocator : allocators)
        {
            allocator.resize(buffer_size);
        }
    }

    /**
     * Swaps the current stack allocator with the next one in the round-robin sequence.
     */
    void swap_buffers()
    {
        stack_index = (stack_index + 1) % N;
        allocators[stack_index].clear();
    }

    /**
     * Allocates a given size from the current stack
     *
     * @param size The size to allocate
     * @return A pointer to the beginning of the allocated memory
     */
    void* alloc(size_t size)
    {
        return allocators[stack_index].alloc(size);
    }

    /**
     * Allocates memory and constructs an object of type T
     *
     * @tparam T The type of object to allocate
     * @tparam Args Constructor argument types
     * @param args Constructor arguments
     * @return Pointer to the new object
     */
    template <typename T, typename... Args>
    T* alloc(Args&&... args)
    {
        void* mem = alloc(sizeof(T));
        return new(mem) T(std::forward<Args>(args)...);
    }

    /**
     * frees an allocation made by the current stack allocator.
     *
     * @param p The pointer to the memory to free. This must be a pointer allocated by the current stack allocator.
     */
    void free(void* p)
    {
        allocators[stack_index].free(p);
    }

    /**
     * Destroys and frees an object of type T
     *
     * @tparam T The type of object to free
     * @param p Pointer to the object
     */
    template <typename T>
    void free(T* p)
    {
        if (p == nullptr)
            return;

        // Call the destructor
        p->~T();

        this->free(reinterpret_cast<void*>(p));
    }

    /**
     * Clears the current stack allocator.
     */
    void clear()
    {
        allocators[stack_index].clear();
    }

    /**
     * Clears a specific stack allocator by index.
     * @param index The index of the stack allocator to clear. Must be in the range [0, N-1].
     */
    void clear(size_t index)
    {
        allocators[stack_index].clear();
    }

    /**
     * Gets the current stack allocator.
     * @return the current stack allocator
     */
    [[nodiscard]] StackAllocator& get_current_allocator()
    {
        return allocators[stack_index];
    }

    /**
     * Gets a specific stack allocator by index.
     *
     * @param index The index of the stack allocator to get. Must be in the range [0, N-1].
     * @return the stack allocator at the specified index
     */
    [[nodiscard]] StackAllocator& get_allocator(size_t index)
    {
        if (index >= N)
        {
            throw std::out_of_range("Index out of range for BufferedAllocator");
        }
        return allocators[index];
    }

private:
    size_t stack_index = 0;
    std::array<StackAllocator, N> allocators;
};

using DoubleBufferedAllocator = BufferedAllocator<2>;

} // portal
