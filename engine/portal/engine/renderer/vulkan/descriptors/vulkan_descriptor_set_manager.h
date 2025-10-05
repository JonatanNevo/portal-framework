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
    VulkanDescriptorSetManager() = default;
    VulkanDescriptorSetManager(const DescriptorSetManagerSpecification& spec, const Ref<VulkanDevice>& device);

    void set_input(StringId name, Ref<UniformBufferSet> buffer) override;
    void set_input(StringId name, Ref<UniformBuffer> buffer) override;
    void set_input(StringId name, Ref<StorageBufferSet> buffer) override;
    void set_input(StringId name, Ref<StorageBuffer> buffer) override;
    void set_input(StringId name, Ref<Texture> texture) override;
    void set_input(StringId name, Ref<Image> image) override;
    void set_input(StringId name, Ref<ImageView> image) override;
    void bind_buffer(StringId name);

    Ref<RefCounted> get_input(StringId name) override;

    bool is_invalidated(size_t set, size_t binding_index) const override;

    bool validate() override;
    void bake() override;

    const shader_reflection::ShaderResourceDeclaration* get_input_declaration(const StringId& name) const;

    void invalidate_and_update(size_t frame_index);

    size_t get_first_set_index() const;
    const std::vector<vk::raii::DescriptorSet>& get_descriptor_sets(size_t frame_index) const;

private:
    void init();
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
    Ref<VulkanDevice> device;

    portal::vulkan::DescriptorAllocator descriptor_allocator;

};

}
