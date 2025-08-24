//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/reference.h"
#include "portal/engine/resources/gpu_context.h"
#include "portal/engine/resources/resource_types.h"
#include "portal/engine/shaders/shader.h"

namespace portal::vulkan
{

// Wrapper for the shader resource with vulkan functions.
class VulkanShader
{
public:
    explicit VulkanShader(const Ref<Shader>& shader);
    std::vector<vk::raii::DescriptorSetLayout> create_descriptor_layouts() const;

    std::vector<vk::PushConstantRange> get_push_constant_range() const;
    vk::raii::ShaderModule create_shader_module() const;

private:
    std::shared_ptr<resources::GpuContext> context;
    Ref<Shader> shader;
};

} // portal