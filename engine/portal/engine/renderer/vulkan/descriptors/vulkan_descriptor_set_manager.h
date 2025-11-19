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

class VulkanDescriptorSetManager final : public DescriptorSetManager
{
public:
    ~VulkanDescriptorSetManager() override;

    static VulkanDescriptorSetManager create(const DescriptorSetManagerSpecification& spec, const VulkanDevice& device);
    static std::unique_ptr<VulkanDescriptorSetManager> create_unique(const DescriptorSetManagerSpecification& spec, const VulkanDevice& device);

    VulkanDescriptorSetManager(const VulkanDescriptorSetManager&) = delete;
    VulkanDescriptorSetManager& operator=(const VulkanDescriptorSetManager&) = delete;

    void set_input(StringId name, const Reference<UniformBufferSet>& buffer) override;
    void set_input(StringId name, const Reference<UniformBuffer>& buffer) override;
    void set_input(StringId name, const Reference<StorageBufferSet>& buffer) override;
    void set_input(StringId name, const Reference<StorageBuffer>& buffer) override;
    void set_input(StringId name, const Reference<Texture>& texture) override;
    void set_input(StringId name, const Reference<Image>& image) override;
    void set_input(StringId name, const Reference<ImageView>& image) override;

    template<typename T>
    Reference<T> get_input(const StringId name)
    {
        return DescriptorSetManager::get_input<T>(name);
    }

    Reference<RendererResource> get_input(StringId name) override;

    bool is_invalidated(size_t set, size_t binding_index) const override;

    bool validate() override;
    void bake() override;

    const shader_reflection::ShaderResourceDeclaration* get_input_declaration(const StringId& name) const;

    void invalidate_and_update(size_t frame_index);

    size_t get_first_set_index() const;
    const std::vector<vk::raii::DescriptorSet>& get_descriptor_sets(size_t frame_index) const;

private:
    VulkanDescriptorSetManager(const DescriptorSetManagerSpecification& spec, const VulkanDevice& device, portal::renderer::vulkan::DescriptorAllocator&& descriptor_allocator);

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
    DescriptorSetManagerSpecification spec;
    const VulkanDevice& device;

    portal::renderer::vulkan::DescriptorAllocator descriptor_allocator;
};

}
