//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "descriptor_layout_builder.h"

namespace portal::vulkan
{
DescriptorLayoutBuilder& DescriptorLayoutBuilder::add_binding(
    const size_t binding,
    const vk::DescriptorType type,
    const vk::ShaderStageFlags shader_stages,
    const size_t count
    )
{
    layout_bindings.push_back(
        {
            .binding = static_cast<uint32_t>(binding),
            .descriptorType = type,
            .descriptorCount = static_cast<uint32_t>(count),
            .stageFlags = shader_stages
        }
        );
    return *this;
}

void DescriptorLayoutBuilder::clear()
{
    layout_bindings.clear();
}

vk::raii::DescriptorSetLayout DescriptorLayoutBuilder::build(const vk::raii::Device& device)
{
    const vk::DescriptorSetLayoutCreateInfo info{
        .flags = {},
        .bindingCount = static_cast<uint32_t>(layout_bindings.size()),
        .pBindings = layout_bindings.data(),
    };

    auto&& set = device.createDescriptorSetLayout(info);
    clear();
    return set;
}
} // portal
