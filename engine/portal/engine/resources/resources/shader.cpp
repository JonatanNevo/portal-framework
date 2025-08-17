//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "shader.h"

namespace portal
{

void Shader::copy_from(const Ref<Resource> other)
{
    auto other_shader = other.as<Shader>();
    descriptor_layout = other_shader->descriptor_layout;
    shader_data = other_shader->shader_data;
}

vk::DescriptorSetLayout Shader::get_descriptor_layout() const
{
    return *descriptor_layout;
}

std::optional<vk::PushConstantRange> Shader::get_push_constant_range(const vk::ShaderStageFlagBits stage) const
{
    auto range = shader_data.at(stage).push_constant_range;
    if (range.size == 0)
        return std::nullopt;
    return range;
}

const std::string& Shader::get_entry_point(vk::ShaderStageFlagBits stage) const
{
    return shader_data.at(stage).entry_point;
}

vk::ShaderModule Shader::get_shader_module(vk::ShaderStageFlagBits stage) const
{
    return *shader_data.at(stage).shader_module;
}
} // portal
