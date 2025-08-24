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

    DescriptorLayoutBuilder& add_binding(size_t binding, vk::DescriptorType type, vk::ShaderStageFlags shader_stages, size_t count = 1);
    void clear();
    vk::raii::DescriptorSetLayout build(const vk::raii::Device& device);
};

} // portal
