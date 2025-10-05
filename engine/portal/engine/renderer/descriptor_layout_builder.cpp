//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "descriptor_layout_builder.h"

#include "vulkan/vulkan_common.h"

namespace portal::vulkan
{
DescriptorLayoutBuilder& DescriptorLayoutBuilder::add_binding(
    const size_t binding,
    const vk::DescriptorType type,
    const vk::ShaderStageFlags shader_stages,
    const size_t count
    )
{
    auto shader_stages_mask = shader_stages;
    if (shader_stages_mask == vk::ShaderStageFlagBits::eAll)
        shader_stages_mask = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

    layout_bindings.push_back(
        {
            .binding = static_cast<uint32_t>(binding),
            .descriptorType = type,
            .descriptorCount = static_cast<uint32_t>(count),
            .stageFlags = shader_stages_mask
        }
        );
    return *this;
}

DescriptorLayoutBuilder& DescriptorLayoutBuilder::set_name(const StringId& layout_name)
{
    name = layout_name;
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

    auto set = device.createDescriptorSetLayout(info);
    clear();

    if (name != INVALID_STRING_ID)
    {
        device.setDebugUtilsObjectNameEXT(
            {
                .objectType = vk::ObjectType::eDescriptorSetLayout,
                .objectHandle = VK_HANDLE_CAST(set),
                .pObjectName = name.string.data()
            }
            );
    }
    return set;
}
} // portal
