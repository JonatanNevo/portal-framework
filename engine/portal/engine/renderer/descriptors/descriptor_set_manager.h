//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/strings/string_id.h"
#include "portal/engine/renderer/descriptors/descriptor_input.h"
#include "portal/engine/resources/resource_reference.h"

namespace portal::renderer
{
class ShaderVariant;

/**
 * @struct DescriptorSetManagerProperties
 * @brief Descriptor set manager configuration
 */
struct DescriptorSetManagerProperties
{
    Reference<ShaderVariant> shader;
    StringId debug_name;

    // Set range to manage
    size_t start_set = 0;
    size_t end_set = std::numeric_limits<size_t>::max();

    Reference<Texture> default_texture;

    // TODO: get from global config
    size_t frame_in_flights;
};

/**
 * @class DescriptorSetManager
 * @brief Manages descriptor set bindings for a shader
 *
 * Binds resources (buffers, textures, images) to shader descriptor sets by name.
 * Tracks invalidation and supports multi-buffering.
 */
class DescriptorSetManager
{
public:
    virtual ~DescriptorSetManager() = default;

    /** @brief Binds uniform buffer set */
    virtual void set_input(StringId name, const Reference<UniformBufferSet>& buffer) = 0;

    /** @brief Binds uniform buffer */
    virtual void set_input(StringId name, const Reference<UniformBuffer>& buffer) = 0;

    /** @brief Binds storage buffer set */
    virtual void set_input(StringId name, const Reference<StorageBufferSet>& buffer) = 0;

    /** @brief Binds storage buffer */
    virtual void set_input(StringId name, const Reference<StorageBuffer>& buffer) = 0;

    /** @brief Binds texture */
    virtual void set_input(StringId name, const Reference<Texture>& texture) = 0;

    /** @brief Binds image */
    virtual void set_input(StringId name, const Reference<Image>& image) = 0;

    /** @brief Binds image view */
    virtual void set_input(StringId name, const Reference<ImageView>& image) = 0;

    /**
     * @brief Gets bound input resource (typed)
     * @tparam T Resource type
     * @param name Binding name
     * @return Resource reference
     */
    template <typename T>
    Reference<T> get_input(const StringId name)
    {
        return reference_cast<T, RendererResource>(get_input(name));
    }

    /**
     * @brief Gets bound input resource
     * @param name Binding name
     * @return Resource reference
     */
    virtual Reference<RendererResource> get_input(StringId name) = 0;

    /**
     * @brief Checks if binding is invalidated
     * @param set Descriptor set index
     * @param binding_index Binding index
     * @return True if invalidated
     */
    virtual bool is_invalidated(size_t set, size_t binding_index) const = 0;

    virtual void invalidate_and_update(size_t frame_index) = 0;

    /** @brief Validates all bindings are set */
    virtual bool validate() = 0;

    /** @brief Finalizes descriptor sets for rendering */
    virtual void bake() = 0;
};
}
