//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/renderer/descriptors/descriptor.h"

namespace portal::renderer
{
/**
 * @class UniformBuffer
 * @brief Uniform buffer descriptor for shader constants
 *
 * Read-only GPU buffer for shader uniform data (transforms, materials, etc.).
 */
class UniformBuffer : public BufferDescriptor
{
public:
    UniformBuffer(const StringId& id) : BufferDescriptor(id, DescriptorResourceType::UniformBuffer) {};
};

/**
 * @class UniformBufferSet
 * @brief Collection of uniform buffers (e.g., per-frame-in-flight)
 *
 * Manages multiple uniform buffer instances for multi-buffering.
 */
class UniformBufferSet : public BufferDescriptor
{
public:
    UniformBufferSet(const StringId& id) : BufferDescriptor(id, DescriptorResourceType::UniformBufferSet) {};

    /**
     * @brief Gets buffer at index
     * @param index Buffer index
     * @return Uniform buffer reference
     */
    virtual Reference<UniformBuffer> get(size_t index) = 0;

    /**
     * @brief Sets buffer at index
     * @param buffer Uniform buffer
     * @param index Buffer index
     */
    virtual void set(const Reference<UniformBuffer>& buffer, size_t index) = 0;
};
}
