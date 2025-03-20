//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once
#include <vector>

#include "base/vulkan_resource.h"

namespace portal::vulkan
{
class Device;
class ShaderModule;
struct ShaderResource;

/**
 * @brief Caches DescriptorSet objects for the shader's set index.
 */
class DescriptorSetLayout final : public VulkanResource<vk::DescriptorSetLayout>
{
public:
    /**
     * @brief Creates a descriptor set layout from a set of resources
     * @param device A valid Vulkan device
     * @param set_index The descriptor set index this layout maps to
     * @param shader_modules The shader modules this set layout will be used for
     * @param resource_set A grouping of shader resources belonging to the same set
     */
    DescriptorSetLayout(
        Device& device,
        uint32_t set_index,
        const std::vector<ShaderModule*>& shader_modules,
        const std::vector<ShaderResource>& resource_set
    );
    DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
    ~DescriptorSetLayout() override;

    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(DescriptorSetLayout&&) = delete;

    uint32_t get_index() const;
    const std::vector<vk::DescriptorSetLayoutBinding>& get_bindings() const;
    std::unique_ptr<vk::DescriptorSetLayoutBinding> get_layout_binding(uint32_t binding_index) const;
    std::unique_ptr<vk::DescriptorSetLayoutBinding> get_layout_binding(const std::string& name) const;
    const std::vector<vk::DescriptorBindingFlagsEXT>& get_binding_flags() const;
    vk::DescriptorBindingFlagsEXT get_layout_binding_flag(uint32_t binding_index) const;
    const std::vector<ShaderModule*>& get_shader_modules() const;

private:
    const uint32_t set_index;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    std::vector<vk::DescriptorBindingFlagsEXT> binding_flags;
    std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings_lookup;
    std::unordered_map<uint32_t, vk::DescriptorBindingFlagsEXT> binding_flags_lookup;
    std::unordered_map<std::string, uint32_t> resources_lookup;
    std::vector<ShaderModule*> shader_modules;
};
} // portal
