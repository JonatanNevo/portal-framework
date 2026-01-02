//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/buffer.h"
#include "portal/engine/renderer/descriptors/descriptor_types.h"
#include "portal/engine/renderer/renderer_resource.h"

namespace portal::renderer
{
/**
 * @class BufferDescriptor
 * @brief Abstract base for uniform and storage buffer descriptors
 *
 * Provides data upload interface and resource type identification.
 * Subclasses include UniformBuffer and StorageBuffer.
 */
class BufferDescriptor : public RendererResource
{
public:
    ~BufferDescriptor() override = default;
    explicit BufferDescriptor(const StringId& id, const DescriptorResourceType type) : RendererResource(id), type(type) {};

    /**
     * @brief Uploads data to buffer
     * @param data CPU buffer to upload
     * @param offset Byte offset in buffer
     */
    virtual void set_data(Buffer data, size_t offset = 0) = 0;

    /** @brief Gets CPU buffer (const) */
    virtual const Buffer& get_data() const = 0;

    /** @brief Gets descriptor resource type */
    DescriptorResourceType get_type() const { return type; };

protected:
    DescriptorResourceType type;
};
}
