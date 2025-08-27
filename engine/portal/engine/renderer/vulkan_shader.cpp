//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_shader.h"

namespace portal::vulkan
{

vk::ShaderStageFlagBits to_shader_stage(const ShaderStage stage)
{
#define CASE(FROM, TO)             \
case portal::ShaderStage::FROM:    \
return vk::ShaderStageFlagBits::TO

    switch (stage)
    {
    CASE(All, eAll);
    CASE(Vertex, eVertex);
    CASE(Fragment, eFragment);
    CASE(Geometry, eGeometry);
    CASE(Compute, eCompute);
    CASE(RayGeneration, eRaygenKHR);
    CASE(Intersection, eIntersectionKHR);
    CASE(AnyHit, eAnyHitKHR);
    CASE(ClosestHit, eClosestHitKHR);
    CASE(Miss, eMissKHR);
    CASE(Callable, eCallableKHR);
    CASE(Mesh, eMeshEXT);
    }

#undef CASE
    return vk::ShaderStageFlagBits::eAll;
}

vk::DescriptorType to_descriptor_type(const DescriptorType type)
{
    switch (type)
    {
    case DescriptorType::Sampler:
        return vk::DescriptorType::eSampler;
    case DescriptorType::CombinedImageSampler:
        return vk::DescriptorType::eCombinedImageSampler;
    case DescriptorType::SampledImage:
        return vk::DescriptorType::eSampledImage;
    case DescriptorType::StorageImage:
        return vk::DescriptorType::eStorageImage;
    case DescriptorType::UniformTexelBuffer:
        return vk::DescriptorType::eUniformTexelBuffer;
    case DescriptorType::StorageTexelBuffer:
        return vk::DescriptorType::eStorageTexelBuffer;
    case DescriptorType::UniformBuffer:
        return vk::DescriptorType::eUniformBuffer;
    case DescriptorType::StorageBuffer:
        return vk::DescriptorType::eStorageBuffer;
    case DescriptorType::UniformBufferDynamic:
        return vk::DescriptorType::eUniformBufferDynamic;
    case DescriptorType::StorageBufferDynamic:
        return vk::DescriptorType::eStorageBufferDynamic;
    case DescriptorType::InputAttachment:
        return vk::DescriptorType::eInputAttachment;
    case DescriptorType::AccelerationStructure:
        return vk::DescriptorType::eAccelerationStructureKHR;
    case DescriptorType::InlineUniformBlock:
        return vk::DescriptorType::eInlineUniformBlockEXT;
    case DescriptorType::Unknown:
        return vk::DescriptorType::eUniformBuffer;
    }
    return vk::DescriptorType::eUniformBuffer;
}

VulkanShader::VulkanShader(const Ref<Shader>& shader, const std::shared_ptr<resources::GpuContext>& context) : context(context), shader(shader)
{}

std::vector<vk::raii::DescriptorSetLayout> VulkanShader::create_descriptor_layouts() const
{
    std::vector<vk::raii::DescriptorSetLayout> output;
    output.reserve(shader->reflection.layouts.size());

    for (const auto& [name, bindings] : shader->reflection.layouts)
    {
        DescriptorLayoutBuilder builder;
        for (auto& binding : bindings)
        {
            builder.add_binding(binding.binding_index, to_descriptor_type(binding.type), to_shader_stage(binding.stage), binding.descriptor_count);
        }
        builder.set_name(name);
        output.push_back(context->create_descriptor_set_layout(builder));
    }

    return std::move(output);
}

std::vector<vk::PushConstantRange> VulkanShader::get_push_constant_range(ShaderStage stage) const
{
    std::vector<vk::PushConstantRange> output;
    output.reserve(shader->reflection.push_constants.size());

    for (auto& push_constant : shader->reflection.push_constants)
    {
        if (stage == push_constant.stage)
        {
            output.push_back(
                vk::PushConstantRange{
                    .stageFlags = to_shader_stage(push_constant.stage),
                    .offset = static_cast<uint32_t>(push_constant.offset),
                    .size = static_cast<uint32_t>(push_constant.size)
                }
                );
        }
    }
    return output;
}

vk::raii::ShaderModule VulkanShader::create_shader_module() const
{
    return context->create_shader_module(shader->code);
}

} // portal
