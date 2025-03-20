//
// Created by Jonatan Nevo on 31/01/2025.
//

#pragma once

#include <memory>

#include "portal/core/assert.h"

namespace portal
{
// A non owning buffer
struct Buffer
{
    void* data;
    size_t size;

    Buffer():
        data(nullptr), size(0) {}

    Buffer(void* data, const size_t size):
        data(data), size(size) {}

    Buffer(const Buffer& other, const size_t size):
        data(other.data), size(size) {}

    static Buffer copy(const Buffer& other)
    {
        Buffer buffer;
        buffer.allocate(other.size);
        memcpy(buffer.data, other.data, other.size);
        return buffer;
    }

    static Buffer copy(const void* data, const size_t size)
    {
        Buffer buffer;
        buffer.allocate(size);
        memcpy(buffer.data, data, size);
        return buffer;
    }

    void allocate(const size_t new_size)
    {
        delete[] static_cast<uint8_t*>(data);
        data = nullptr;

        if (new_size == 0)
            return;

        data = new uint8_t[new_size];
        this->size = new_size;
    }

    void release()
    {
        delete[] static_cast<uint8_t*>(data);
        data = nullptr;
        size = 0;
    }

    void zero_initialize() const
    {
        if (data)
            memset(data, 0, size);
    }

    template <typename T>
    T& read(const size_t offset = 0)
    {
        PORTAL_CORE_ASSERT(offset >= size, "Buffer overflow");
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(data) + offset);
    }

    template <typename T>
    const T& read(const size_t offset = 0) const
    {
        PORTAL_CORE_ASSERT(offset >= size, "Buffer overflow");
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(data) + offset);
    }

    [[nodiscard]] uint8_t* read_bytes(const size_t bytes_size, const size_t offset) const
    {
        PORTAL_ASSERT(offset + bytes_size > size, "Buffer overflow");
        auto* buffer = new uint8_t[bytes_size];
        memcpy(buffer, static_cast<uint8_t*>(data) + offset, bytes_size);
        return buffer;
    }

    void write(const void* new_data, size_t data_size, size_t offset = 0) const
    {
        memcpy(static_cast<uint8_t*>(data) + offset, new_data, data_size);
    }

    explicit operator bool() const
    {
        return data;
    }

    uint8_t& operator[](const size_t index) const
    {
        return static_cast<uint8_t*>(data)[index];
    }

    template <typename T>
    T* as() const
    {
        return static_cast<T*>(data);
    }

    [[nodiscard]] inline size_t get_size() const
    {
        return size;
    }
};
}
