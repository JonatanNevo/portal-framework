//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vulkan/vulkan.hpp>

#include "portal/core/buffer.h"
#include "portal/core/strings/string_id.h"
#include "portal/engine/renderer/descriptors/descriptor_types.h"
#include "portal/engine/renderer/descriptors/base_descriptor.h"

namespace portal::renderer
{
/**
 * @class BufferDescriptor
 * @brief Abstract base for uniform and storage buffer descriptors
 *
 * Provides data upload interface and resource type identification.
 * Subclasses include UniformBuffer and StorageBuffer.
 */
class BufferDescriptor: public BaseDescriptor
{
public:
    BufferDescriptor(const StringId& id, const DescriptorResourceType type) : id(id), type(type) {}

    template <class T>
    void set_data_typed(const vk::ArrayProxy<T>& object, const size_t offset = 0)
    {
        set_data(Buffer{reinterpret_cast<const uint8_t*>(object.data()), object.size() * sizeof(T)}, offset);
    }

    /**
     * @brief Uploads data to buffer
     * @param data CPU buffer to upload
     * @param offset Byte offset in buffer
     */
    virtual void set_data(Buffer data, size_t offset = 0) = 0;

    /** @brief Gets CPU buffer (const) */
    [[nodiscard]] virtual const Buffer& get_data() const = 0;

    /** @brief Gets descriptor resource type */
    [[nodiscard]] DescriptorResourceType get_type() const { return type; };

    [[nodiscard]] StringId get_id() const { return id; }

protected:
    StringId id;
    DescriptorResourceType type;
};
}
