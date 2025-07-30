//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "descriptor_layout_builder.h"

namespace portal::vulkan {
DescriptorLayoutBuilder& DescriptorLayoutBuilder::add_binding(const uint32_t binding, const vk::DescriptorType type) {
    layout_bindings.push_back({
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = 1,
    });
    return *this;
}

void DescriptorLayoutBuilder::clear() {
    layout_bindings.clear();
}

vk::raii::DescriptorSetLayout DescriptorLayoutBuilder::build(vk::raii::Device& device, vk::ShaderStageFlags shader_stages) {
    for (auto& binding: layout_bindings) {
        binding.stageFlags |= shader_stages;
    }

    const vk::DescriptorSetLayoutCreateInfo info {
        .flags = {},
        .bindingCount = static_cast<uint32_t>(layout_bindings.size()),
        .pBindings = layout_bindings.data(),
    };

    return device.createDescriptorSetLayout(info);
}
} // portal