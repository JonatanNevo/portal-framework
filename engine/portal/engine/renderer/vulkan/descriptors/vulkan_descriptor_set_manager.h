//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <set>

#include "portal/engine/renderer/descriptor_allocator.h"
#include "portal/engine/renderer/descriptors/descriptor_set_manager.h"
#include "portal/engine/renderer/shaders/shader_types.h"

namespace portal::renderer::vulkan
{
class VulkanDevice;

/**
 * @class VulkanDescriptorSetManager
 * @brief Vulkan descriptor set manager with automatic invalidation tracking
 *
 * Binds resources to shader descriptor sets by name using shader reflection.
 * Manages descriptor allocation, updates, and multi-buffering.
 */
class VulkanDescriptorSetManager final : public DescriptorSetManager
{
public:
    ~VulkanDescriptorSetManager() override;

    /**
     * @brief Creates descriptor set manager
     * @param properties Configuration
     * @param device Vulkan device
     * @return Descriptor set manager
     */
    static VulkanDescriptorSetManager create(const DescriptorSetManagerProperties& properties, const VulkanDevice& device);

    /**
     * @brief Creates unique descriptor set manager
     * @param properties Configuration
     * @param device Vulkan device
     * @return Unique pointer to manager
     */
    static std::unique_ptr<VulkanDescriptorSetManager> create_unique(const DescriptorSetManagerProperties& properties, const VulkanDevice& device);

    VulkanDescriptorSetManager(const VulkanDescriptorSetManager&) = delete;
    VulkanDescriptorSetManager& operator=(const VulkanDescriptorSetManager&) = delete;

    /** @brief Binds uniform buffer set */
    void set_input(StringId name, const Reference<UniformBufferSet>& buffer) override;

    /** @brief Binds uniform buffer */
    void set_input(StringId name, const Reference<UniformBuffer>& buffer) override;

    /** @brief Binds storage buffer set */
    void set_input(StringId name, const Reference<StorageBufferSet>& buffer) override;

    /** @brief Binds storage buffer */
    void set_input(StringId name, const Reference<StorageBuffer>& buffer) override;

    /** @brief Binds texture */
    void set_input(StringId name, const Reference<Texture>& texture) override;

    /** @brief Binds image */
    void set_input(StringId name, const Reference<Image>& image) override;

    /** @brief Binds image view */
    void set_input(StringId name, const Reference<ImageView>& image) override;

    /**
     * @brief Gets bound input resource (typed)
     * @tparam T Resource type
     * @param name Binding name
     * @return Resource reference
     */
    template <typename T>
    Reference<T> get_input(const StringId name)
    {
        return DescriptorSetManager::get_input<T>(name);
    }

    /**
     * @brief Gets bound input resource
     * @param name Binding name
     * @return Resource reference
     */
    Reference<RendererResource> get_input(StringId name) override;

    /**
     * @brief Checks if binding is invalidated
     * @param set Descriptor set index
     * @param binding_index Binding index
     * @return True if invalidated
     */
    bool is_invalidated(size_t set, size_t binding_index) const override;

    /** @brief Validates all bindings are set */
    bool validate() override;

    /** @brief Finalizes descriptor sets for rendering */
    void bake() override;

    /**
     * @brief Gets resource declaration from shader reflection
     * @param name Resource name
     * @return Reflection data or nullptr
     */
    const shader_reflection::ShaderResourceDeclaration* get_input_declaration(const StringId& name) const;

    /**
     * @brief Updates invalidated bindings for frame
     * @param frame_index Frame-in-flight index
     */
    void invalidate_and_update(size_t frame_index);

    /** @brief Gets first managed descriptor set index */
    size_t get_first_set_index() const;

    /**
     * @brief Gets descriptor sets for frame
     * @param frame_index Frame-in-flight index
     * @return Descriptor sets
     */
    const std::vector<vk::raii::DescriptorSet>& get_descriptor_sets(size_t frame_index) const;

private:
    VulkanDescriptorSetManager(
        const DescriptorSetManagerProperties& properties,
        const VulkanDevice& device,
        portal::renderer::vulkan::DescriptorAllocator&& descriptor_allocator
    );

    VulkanDescriptorSetManager& init();
    std::set<size_t> get_buffer_sets();

public:
    std::unordered_map<size_t, std::unordered_map<size_t, DescriptorInput>> input_resources;
    std::unordered_map<size_t, std::unordered_map<size_t, DescriptorInput>> invalid_input_resources;
    std::unordered_map<StringId, shader_reflection::ShaderResourceDeclaration> input_declarations;

    //per frame in flight
    std::vector<std::vector<vk::raii::DescriptorSet>> descriptor_sets;

    struct WriteDescriptor
    {
        vk::WriteDescriptorSet write_descriptor_set;
        std::vector<void*> resource_handles;
    };

    std::vector<std::unordered_map<size_t, std::unordered_map<size_t, WriteDescriptor>>> write_descriptors_map;

private:
    DescriptorSetManagerProperties properties;
    const VulkanDevice& device;

    portal::renderer::vulkan::DescriptorAllocator descriptor_allocator;
};
}
