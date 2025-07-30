//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory>

#include "debug/assert.h"

namespace portal
{
// A non owning buffer
struct Buffer
{
    const void* data;
    size_t size;

    Buffer():
        data(nullptr),
        size(0),
        allocated(false) {}

    Buffer(nullptr_t):
        data(nullptr),
        size(0),
        allocated(false) {}

    Buffer(const void* data, const size_t size):
        data(data),
        size(size),
        allocated(false) {}

    Buffer(const Buffer& other): Buffer(other, other.size) {}

    Buffer(const Buffer& other, const size_t size):
        data(other.data),
        size(size),
        allocated(false) {}

    Buffer(Buffer&& other) noexcept:
        data(std::exchange(other.data, nullptr)),
        size(std::exchange(other.size, 0)),
        allocated(std::exchange(other.allocated, false))
    {}

    Buffer& operator=(const Buffer& other)
    {
        if (this == &other)
            return *this;

        if (allocated)
            release();

        data = other.data;
        size = other.size;
        return *this;
    }

    Buffer& operator=(Buffer&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (allocated)
            release();

        data = std::exchange(other.data, nullptr);
        size = std::exchange(other.size, 0);
        allocated = std::exchange(other.allocated, false);
        return *this;
    }

    ~Buffer()
    {
        if (allocated)
            release();
    }

    template<typename T, typename... Args>
    [[nodiscard]] PORTAL_FORCE_INLINE static Buffer create(Args&&... args)
    {
        Buffer&& buffer = allocate(sizeof(T));
        new (buffer.data_ptr()) T(std::forward<Args>(args)...);
        return std::move(buffer);
    }

    [[nodiscard]] PORTAL_FORCE_INLINE static Buffer copy(const Buffer& other)
    {
        Buffer&& buffer = allocate(other.size);
        memcpy(const_cast<void*>(buffer.data), other.data, other.size);
        return std::move(buffer);
    }

    [[nodiscard]] PORTAL_FORCE_INLINE static Buffer copy(const void* data, const size_t size)
    {
        Buffer&& buffer = allocate(size);
        memcpy(const_cast<void*>(buffer.data), data, size);
        return std::move(buffer);
    }

    template <typename T>
    [[nodiscard]] PORTAL_FORCE_INLINE static Buffer copy(T& t)
    {
        return copy(&t, sizeof(T));
    }

    [[nodiscard]] PORTAL_FORCE_INLINE static Buffer allocate(const size_t new_size)
    {
        if (new_size == 0)
            return Buffer{};

        return Buffer{
            new uint8_t[new_size],
            new_size,
            true
        };
    }

    PORTAL_FORCE_INLINE void release()
    {
        PORTAL_ASSERT(allocated, "Releasing unallocated buffer");
        if (data)
            delete[] static_cast<uint8_t*>(data_ptr());
        data = nullptr;
        size = 0;
        allocated = false;
    }

    PORTAL_FORCE_INLINE void zero_initialize() const
    {
        if (data)
            memset(const_cast<void*>(data), 0, size);
    }

    template <typename T>
    PORTAL_FORCE_INLINE T& read(const size_t offset = 0)
    {
        PORTAL_ASSERT(offset <= size, "Buffer overflow");
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(data_ptr()) + offset);
    }

    template <typename T>
    PORTAL_FORCE_INLINE const T& read(const size_t offset = 0) const
    {
        PORTAL_ASSERT(offset <= size, "Buffer overflow");
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(const_cast<void*>(data)) + offset);
    }

    PORTAL_FORCE_INLINE void write(const void* new_data, const size_t data_size, const size_t offset = 0)
    {
        PORTAL_ASSERT(offset + data_size <= size, "Buffer overflow");
        memcpy(static_cast<uint8_t*>(const_cast<void*>(data)) + offset, new_data, data_size);
    }

    PORTAL_FORCE_INLINE explicit operator bool() const
    {
        return data;
    }

    PORTAL_FORCE_INLINE uint8_t& operator[](const size_t index) const
    {
        return static_cast<uint8_t*>(const_cast<void*>(data))[index];
    }

    template <typename T> requires std::is_pointer_v<T>
    PORTAL_FORCE_INLINE T as() const
    {
        return static_cast<T>(const_cast<void*>(data));
    }

    PORTAL_FORCE_INLINE void* data_ptr()
    {
        return const_cast<void*>(data);
    }

    [[nodiscard]] PORTAL_FORCE_INLINE const void* data_ptr() const
    {
        return data;
    }

    [[nodiscard]] PORTAL_FORCE_INLINE bool is_allocated() const
    {
        return allocated;
    }

private:
    Buffer(const void* data, const size_t size, const bool allocated):
        data(data),
        size(size),
        allocated(allocated)
    {
        PORTAL_ASSERT(data || size == 0, "Buffer data cannot be null if size is not zero");
    }

private:
    bool allocated;
};
}
