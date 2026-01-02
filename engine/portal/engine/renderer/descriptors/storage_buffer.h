//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/renderer/descriptors/descriptor.h"
#include "portal/core/strings/string_id.h"

namespace portal::renderer
{
/**
 * @struct StorageBufferProperties
 * @brief Storage buffer creation parameters
 */
struct StorageBufferProperties
{
    size_t size;
    bool gpu_only = true;
    StringId debug_name;
};

/**
 * @class StorageBuffer
 * @brief Storage buffer descriptor for read-write shader data
 *
 * GPU buffer supporting shader read-write access (compute outputs, particle data, etc.).
 * Supports resizing.
 */
class StorageBuffer : public BufferDescriptor
{
public:
    explicit StorageBuffer(const StringId& id) : BufferDescriptor(id, DescriptorResourceType::StorageBuffer) {};

    /**
     * @brief Resizes buffer (recreates GPU allocation)
     * @param new_size New size in bytes
     */
    virtual void resize(size_t new_size) = 0;
};

/**
 * @class StorageBufferSet
 * @brief Collection of storage buffers (e.g., per-frame-in-flight)
 *
 * Manages multiple storage buffer instances for multi-buffering.
 */
class StorageBufferSet : public BufferDescriptor
{
public:
    explicit StorageBufferSet(const StringId& id) : BufferDescriptor(id, DescriptorResourceType::StorageBufferSet) {};

    /**
     * @brief Gets buffer at index
     * @param index Buffer index
     * @return Storage buffer reference
     */
    virtual Reference<StorageBuffer> get(size_t index) = 0;

    /**
     * @brief Sets buffer at index
     * @param buffer Storage buffer
     * @param index Buffer index
     */
    virtual void set(const Reference<StorageBuffer>& buffer, size_t index) = 0;
};
}
