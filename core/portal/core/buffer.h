//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <memory>
#include <cstring>

#include "common.h"
#include "debug/assert.h"

namespace portal
{
// A non owning buffer
struct Buffer
{
    const void* data;
    size_t size;

    Buffer() :
        data(nullptr),
        size(0),
        allocated(false) {}

    Buffer(std::nullptr_t) :
        data(nullptr),
        size(0),
        allocated(false) {}

    Buffer(const void* data, const size_t size) :
        data(data),
        size(size),
        allocated(false) {}

    Buffer(const Buffer& other) : Buffer(other, 0, other.size) {}

    Buffer(const Buffer& other, const size_t size) : Buffer(other, 0, size) {}

    Buffer(const Buffer& other, const size_t offest, const size_t size) :
        data(static_cast<const uint8_t*>(other.data) + offest),
        size(size),
        allocated(false)
    {}

    Buffer(Buffer&& other) noexcept :
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

    bool operator==(std::nullptr_t) const
    {
        return data == nullptr;
    }

    template <typename T, typename... Args>
    [[nodiscard]] static Buffer create(Args&&... args)
    {
        Buffer&& buffer = allocate(sizeof(T));
        new(buffer.data_ptr()) T(std::forward<Args>(args)...);
        return std::move(buffer);
    }

    [[nodiscard]] static Buffer copy(const Buffer& other, const size_t offset = 0)
    {
        PORTAL_ASSERT(offset + other.size <= other.size, "Buffer overflow");
        Buffer&& buffer = allocate(other.size);
        std::memcpy(static_cast<std::byte*>(const_cast<void*>(buffer.data)) + offset, other.data, other.size);
        return std::move(buffer);
    }

    [[nodiscard]] static Buffer copy(const void* data, const size_t size)
    {
        Buffer&& buffer = allocate(size);
        std::memcpy(const_cast<void*>(buffer.data), data, size);
        return std::move(buffer);
    }

    template <typename T>
    [[nodiscard]] static Buffer copy_from(T& t)
    {
        return copy(&t, sizeof(T));
    }

    [[nodiscard]] static Buffer allocate(const size_t new_size)
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
        if (data)
            delete[] static_cast<uint8_t*>(data_ptr());
        data = nullptr;
        size = 0;
        allocated = false;
    }

    PORTAL_FORCE_INLINE void resize(const size_t new_size)
    {
        if (new_size == size)
            return;

        Buffer&& new_buffer = allocate(new_size);
        std::memcpy(new_buffer.data_ptr(), data, std::min(new_size, size));
        release();

        data = new_buffer.data;
        size = new_buffer.size;
        allocated = std::exchange(new_buffer.allocated, false);
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

    PORTAL_FORCE_INLINE void write(const Buffer& other, const size_t offset = 0) const
    {
        PORTAL_ASSERT(offset + other.size <= size, "Buffer overflow");
        std::memmove(static_cast<uint8_t*>(const_cast<void*>(data)) + offset, other.data, other.size);
    }

    PORTAL_FORCE_INLINE void write(const void* new_data, const size_t data_size, const size_t offset = 0) const
    {
        PORTAL_ASSERT(offset + data_size <= size, "Buffer overflow");
        std::memmove(static_cast<uint8_t*>(const_cast<void*>(data)) + offset, new_data, data_size);
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

    std::string as_string() const
    {
        return std::string(as<const char*>(), size);
    }

private:
    Buffer(const void* data, const size_t size, const bool allocated) :
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
