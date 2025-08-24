//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_shader.h"

namespace portal::vulkan
{

vk::ShaderStageFlagBits to_shader_stage(const ShaderStage stage)
{
    switch (stage)
    {
    case ShaderStage::All:
        return vk::ShaderStageFlagBits::eAll;
    case ShaderStage::Vertex:
        return vk::ShaderStageFlagBits::eVertex;
    case ShaderStage::Fragment:
        return vk::ShaderStageFlagBits::eFragment;
    case ShaderStage::Geometry:
        return vk::ShaderStageFlagBits::eGeometry;
    case ShaderStage::Compute:
        return vk::ShaderStageFlagBits::eCompute;
    case ShaderStage::RayGeneration:
        return vk::ShaderStageFlagBits::eRaygenKHR;
    case ShaderStage::Intersection:
        return vk::ShaderStageFlagBits::eIntersectionKHR;
    case ShaderStage::AnyHit:
        return vk::ShaderStageFlagBits::eAnyHitKHR;
    case ShaderStage::ClosestHit:
        return vk::ShaderStageFlagBits::eClosestHitKHR;
    case ShaderStage::Miss:
        return vk::ShaderStageFlagBits::eMissKHR;
    case ShaderStage::Callable:
        return vk::ShaderStageFlagBits::eCallableKHR;
    case ShaderStage::Mesh:
        return vk::ShaderStageFlagBits::eMeshEXT;
    }
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

VulkanShader::VulkanShader(const Ref<Shader>& shader) : shader(shader)
{}

std::vector<vk::raii::DescriptorSetLayout> VulkanShader::create_descriptor_layouts() const
{
    DescriptorLayoutBuilder builder;
    std::vector<vk::raii::DescriptorSetLayout> output;
    output.reserve(shader->reflection.layouts.size());

    for (const auto& [_, bindings] : shader->reflection.layouts)
    {
        for (auto& binding : bindings)
        {
            builder.add_binding(binding.binding_index, to_descriptor_type(binding.type), to_shader_stage(binding.stage), binding.descriptor_count);
            output.push_back(context->create_descriptor_set_layout(builder));

            builder.clear();
        }
    }

    return output;
}

std::vector<vk::PushConstantRange> VulkanShader::get_push_constant_range() const
{
    std::vector<vk::PushConstantRange> output;
    output.reserve(shader->reflection.push_constants.size());

    for (auto& push_constant : shader->reflection.push_constants)
    {
        output.push_back(
            vk::PushConstantRange{
                .stageFlags = to_shader_stage(push_constant.stage),
                .offset = static_cast<uint32_t>(push_constant.offset),
                .size = static_cast<uint32_t>(push_constant.size)
            }
            );
    }
    return output;
}

vk::raii::ShaderModule VulkanShader::create_shader_module() const
{
    return context->create_shader_module(shader->code);
}

} // portal
