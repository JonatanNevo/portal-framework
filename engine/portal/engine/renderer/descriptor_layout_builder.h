//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace portal::vulkan
{

class DescriptorLayoutBuilder
{
public:
    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;

    DescriptorLayoutBuilder& add_binding(uint32_t binding, vk::DescriptorType type);
    void clear();
    vk::raii::DescriptorSetLayout build(const vk::raii::Device& device, vk::ShaderStageFlags shader_stages);
};

} // portal
