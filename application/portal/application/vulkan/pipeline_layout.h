//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once
#include "base/vulkan_resource.h"
#include "shaders/shader_module.h"

namespace portal::vulkan
{
class Device;
class DescriptorSetLayout;

class PipelineLayout final : public VulkanResource<vk::PipelineLayout>
{
public:
    PipelineLayout(Device& device, const std::vector<ShaderModule*>& shader_modules);
    PipelineLayout(PipelineLayout&& other) noexcept;
    ~PipelineLayout() override;

    PipelineLayout(const PipelineLayout&) = delete;
    PipelineLayout& operator=(const PipelineLayout&) = delete;
    PipelineLayout& operator=(PipelineLayout&& other) noexcept = delete;

    const DescriptorSetLayout& get_descriptor_set_layout(uint32_t set_index) const;
    vk::ShaderStageFlags get_push_constant_range_stage(uint32_t size, uint32_t offset = 0) const;
    std::vector<ShaderResource> get_resources(
        const ShaderResourceType& type = ShaderResourceType::All,
        vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eAll
    ) const;
    const std::vector<ShaderModule*>& get_shader_modules() const;
    std::unordered_map<uint32_t, std::vector<ShaderResource>> get_shader_sets() const;

    bool has_descriptor_set_layout(uint32_t set_index) const;

private:
    std::vector<ShaderModule*> shader_modules;
    std::unordered_map<std::string, ShaderResource> shader_resources; // The shader resources that this pipeline layout uses, indexed by their name
    std::unordered_map<uint32_t, std::vector<ShaderResource>> shader_sets; // A map of each set and the resources it owns used by the pipeline layout
    std::vector<DescriptorSetLayout*> descriptor_set_layouts;
};
} // portal
