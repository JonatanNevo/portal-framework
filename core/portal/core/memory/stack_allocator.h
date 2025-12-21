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
 * Bump/linear allocator providing O(1) allocation via pointer increment.
 *
 * StackAllocator maintains a contiguous buffer and a 'top' marker. Allocation
 * simply increments the top pointer (bounds-checked), making it extremely fast
 * compared to general-purpose allocators. This is ideal for temporary allocations
 * with similar lifetimes, such as per-frame data in game engines.
 *
 * The key feature is marker-based bulk deallocation: capture a marker with
 * get_marker(), perform any number of allocations, then free_to_marker() to
 * instantly free everything allocated since that marker by resetting the top
 * pointer.
 *
 * Individual free() is supported but only efficient for LIFO (stack) order.
 * The allocator tracks allocations in an internal map, so marker-based freeing
 * is preferred for performance.
 *
 * Thread Safety: NOT thread-safe. Designed for single-threaded contexts where
 * one thread owns the allocator for its temporary allocations.
 *
 * Example - Per-frame temporary allocations:
 * @code
 * StackAllocator frame_alloc(1024 * 1024);  // 1MB buffer
 *
 * void process_frame() {
 *     auto marker = frame_alloc.get_marker();
 *
 *     // Allocate temporary data for this frame
 *     auto* entities = frame_alloc.alloc<EntityList>(100);
 *     auto* transform_buffer = frame_alloc.alloc<Transform>(entities->size());
 *
 *     // ... use data throughout frame processing ...
 *
 *     // Free all frame allocations instantly
 *     frame_alloc.free_to_marker(marker);
 * }
 * @endcode
 *
 * Important Notes:
 * - Markers become invalid after resize() or clear()
 * - Default size is 1KB (see implementation) - size appropriately for your use case
 * - Allocation throws std::bad_alloc when buffer is full
 * - Alignment: No alignment guarantees beyond byte alignment. Types requiring
 *   stricter alignment (SSE vectors, cache-line alignment) may exhibit undefined
 *   behavior. For aligned types, use aligned_alloc or posix_memalign instead.
 * - Complements mimalloc (the global allocator) for specific high-performance patterns
 *
 * @see PoolAllocator for fixed-size object pools with arbitrary free/reuse patterns
 * @see BufferedAllocator for multi-frame buffering with round-robin StackAllocators
 */
class StackAllocator
{
public:
    /**
     * Type alias for position markers used by get_marker() and free_to_marker().
     * Represents a position in the stack's buffer (offset in bytes from buffer start).
     */
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
     * Frees all allocations made after the specified marker.
     *
     * This instantly resets the 'top' pointer to the marker position, freeing
     * all memory allocated since get_marker() was called. This is the preferred
     * deallocation method for bulk freeing as it has zero iteration cost.
     *
     * WARNING: This does not update the internal allocations map. Do not call
     * free() on individual allocations made after this marker - the behavior
     * is undefined. Use marker-based deallocation exclusively for bulk freeing,
     * or use clear() to reset both the top pointer and allocations map.
     *
     * @param m The marker obtained from get_marker(). Must be a valid marker
     *          from this allocator instance. Markers are invalidated by resize()
     *          or clear() operations.
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
 * Multi-buffered allocator wrapping N StackAllocators for round-robin frame buffering.
 *
 * BufferedAllocator manages N independent StackAllocators and rotates through them
 * using swap_buffers(). This enables safe multi-frame data persistence, essential
 * for scenarios where frame N data must remain valid while frame N+1 is prepared,
 * such as GPU resource updates that execute asynchronously.
 *
 * Each swap_buffers() call advances to the next allocator (wrapping at N) and
 * clears it, ensuring old data from N frames ago is freed. With double buffering
 * (N=2), frame N writes to buffer 0 while GPU reads frame N-1 from buffer 1.
 * With triple buffering (N=3), an additional frame of overlap is provided for
 * even more CPU/GPU parallelism.
 *
 * All allocation and deallocation operations (alloc(), free()) operate on the
 * current buffer selected by the internal stack_index. Use get_allocator(index)
 * to access specific buffers if needed.
 *
 * Example - Double-buffered GPU resource updates:
 * @code
 * BufferedAllocator<2> gpu_staging(4 * 1024 * 1024);  // 4MB per buffer
 *
 * void render_frame() {
 *     // Allocate staging data in current buffer
 *     auto* vertices = gpu_staging.alloc<Vertex>(vertex_count);
 *     auto* uniforms = gpu_staging.alloc<UniformData>();
 *
 *     // Fill data and upload to GPU
 *     fill_vertex_buffer(vertices, vertex_count);
 *     gpu_upload(vertices, sizeof(Vertex) * vertex_count);
 *
 *     // Submit GPU commands referencing this buffer's data
 *     submit_render_commands();
 *
 *     // Swap to next buffer (clears it, but previous buffer remains valid
 *     // for GPU to read while we prepare the next frame)
 *     gpu_staging.swap_buffers();
 * }
 * @endcode
 *
 * Example - Triple buffering for maximum overlap:
 * @code
 * BufferedAllocator<3> triple_buffer(2 * 1024 * 1024);
 * // Frame N-2 data being read by GPU
 * // Frame N-1 data in flight
 * // Frame N being written by CPU
 * @endcode
 *
 * @tparam N Number of buffers to rotate through. Must be at least 2. Common
 *           values are 2 (double buffering) or 3 (triple buffering).
 *
 * @see StackAllocator for the underlying allocator being buffered
 * @see DoubleBufferedAllocator type alias for BufferedAllocator<2>
 */
template <unsigned int N> requires (N >= 2)
class BufferedAllocator
{
public:
    /**
     * Default constructor creating N stack allocators with default size.
     */
    BufferedAllocator() = default;

    /**
     * Constructs N stack allocators each with the specified buffer size.
     * @param buffer_size Size in bytes for each of the N internal StackAllocators
     */
    explicit BufferedAllocator(const size_t buffer_size)
    {
        for (auto& allocator : allocators)
        {
            allocator.resize(buffer_size);
        }
    }

    /**
     * Advances to the next buffer in round-robin sequence and clears it.
     *
     * This increments the internal stack_index (wrapping at N) and clears the
     * newly selected buffer, making it ready for the next frame's allocations.
     * Previous buffers remain untouched, preserving their data for in-flight
     * operations (like GPU reads).
     *
     * Call this once per frame at frame boundaries to rotate buffers.
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
     * Note: Current implementation ignores the parameter and clears the current allocator.
     */
    void clear(size_t)
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
