//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_shader.h"

#include <ranges>

#include "portal/engine/renderer/descriptor_layout_builder.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/vulkan_physical_device.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"

namespace portal::renderer::vulkan
{

static auto logger = Log::get_logger("Vulkan");

VulkanShader::VulkanShader(StringId id) : Shader(id)
{}

void VulkanShader::initialize(const Ref<VulkanContext>& new_context)
{
    context = new_context;
}

WeakRef<ShaderVariant> VulkanShader::get_shader(uint64_t shader_hash)
{
    if (!shaders.contains(shader_hash))
    {
        LOGGER_ERROR("Shader variant not found: {}", id);
        return {};
    }

    if (!variant_map.contains(shader_hash))
    {
        auto [shader_iter, success] = variant_map.emplace(shader_hash, Ref<VulkanShaderVariant>::create(id, context));
        if (!success)
        {
            LOGGER_ERROR("Failed to create shader variant: {}", id);
            return {};
        }
        auto shader = shader_iter->second;

        auto shader_data = shaders[shader_hash];
        shader->load(std::move(shader_data));
    }

    return variant_map[shader_hash].as<ShaderVariant>();
}

VulkanShaderVariant::VulkanShaderVariant(const StringId& name, const Ref<VulkanContext>& context) : code(code),
    name(name),
    device(context->get_device())
{}

VulkanShaderVariant::~VulkanShaderVariant()
{
    release();
}

void VulkanShaderVariant::release()
{
    shader_modules.clear();
    shader_stage_create_infos.clear();
    descriptor_layouts.clear();
    type_counts.clear();
}

StringId VulkanShaderVariant::get_name() const
{
    return name;
}

std::unordered_map<StringId, vk::WriteDescriptorSet>& VulkanShaderVariant::get_write_descriptor_sets(size_t set_index)
{
    return write_descriptor_sets[set_index];
}

vk::DescriptorSetLayout VulkanShaderVariant::get_descriptor_layout(const size_t set_index) const
{
    return *descriptor_layouts[set_index];
}

std::vector<vk::DescriptorSetLayout> VulkanShaderVariant::get_descriptor_layouts() const
{
    return descriptor_layouts | std::ranges::views::transform([](const auto& layout) { return *layout; }) | std::ranges::to<std::vector>();
}

const std::vector<vk::PushConstantRange>& VulkanShaderVariant::get_push_constant_ranges() const
{
    return push_constant_ranges;
}

const std::unordered_map<StringId, shader_reflection::ShaderResourceDeclaration>& VulkanShaderVariant::get_shader_resources() const
{
    return code.reflection.resources;
}

const ShaderReflection& VulkanShaderVariant::get_reflection() const
{
    return code.reflection;
}

const std::vector<vk::PipelineShaderStageCreateInfo>& VulkanShaderVariant::get_shader_stage_create_infos() const
{
    return shader_stage_create_infos;
}

void VulkanShaderVariant::load(CompiledShader&& compiled_shader)
{
    code = std::move(compiled_shader);
    PORTAL_ASSERT(code.code != nullptr, "Shader code is null");

    shader_stage_create_infos.clear();
    std::string module_name;
    for (auto& [stage, entry_point] : code.reflection.stages)
    {
        auto& shader_module = shader_modules.emplace_back(device->create_shader_module(code.code));
        shader_stage_create_infos.emplace_back(
            vk::PipelineShaderStageCreateInfo{
                .stage = to_shader_stage(stage),
                .module = shader_module,
                .pName = entry_point.c_str()
            }
            );
    }

    create_descriptors();

    for (auto& [stage, offset, size] : code.reflection.push_constants)
    {
        push_constant_ranges.emplace_back(
            vk::PushConstantRange{
                .stageFlags = to_shader_stage(stage),
                .offset = static_cast<uint32_t>(offset),
                .size = static_cast<uint32_t>(size)
            }
            );
    }
}

void VulkanShaderVariant::create_descriptors()
{
    type_counts.clear();
    for (uint32_t set = 0; set < code.reflection.descriptor_sets.size(); ++set)
    {
        auto& shader_descriptor_set = code.reflection.descriptor_sets[set];

        if (!shader_descriptor_set.uniform_buffers.empty())
        {
            type_counts[set].emplace_back(
                vk::DescriptorPoolSize{
                    .type = vk::DescriptorType::eUniformBuffer,
                    .descriptorCount = static_cast<uint32_t>(shader_descriptor_set.uniform_buffers.size())
                }
                );
        }

        if (!shader_descriptor_set.storage_buffers.empty())
        {
            type_counts[set].emplace_back(
                vk::DescriptorPoolSize{
                    .type = vk::DescriptorType::eStorageBuffer,
                    .descriptorCount = static_cast<uint32_t>(shader_descriptor_set.storage_buffers.size())
                }
                );
        }

        if (!shader_descriptor_set.image_samplers.empty())
        {
            type_counts[set].emplace_back(
                vk::DescriptorPoolSize{
                    .type = vk::DescriptorType::eCombinedImageSampler,
                    .descriptorCount = static_cast<uint32_t>(shader_descriptor_set.image_samplers.size())
                }
                );
        }

        if (!shader_descriptor_set.images.empty())
        {
            type_counts[set].emplace_back(
                vk::DescriptorPoolSize{
                    .type = vk::DescriptorType::eSampledImage,
                    .descriptorCount = static_cast<uint32_t>(shader_descriptor_set.images.size())
                }
                );
        }

        if (!shader_descriptor_set.samplers.empty())
        {
            type_counts[set].emplace_back(
                vk::DescriptorPoolSize{
                    .type = vk::DescriptorType::eSampler,
                    .descriptorCount = static_cast<uint32_t>(shader_descriptor_set.samplers.size())
                }
                );
        }

        if (!shader_descriptor_set.storage_images.empty())
        {
            type_counts[set].emplace_back(
                vk::DescriptorPoolSize{
                    .type = vk::DescriptorType::eStorageImage,
                    .descriptorCount = static_cast<uint32_t>(shader_descriptor_set.storage_images.size())
                }
                );
        }

        //////////////////////////////////////////////////////////////////////
        // Descriptor Set Layout
        //////////////////////////////////////////////////////////////////////

        portal::vulkan::DescriptorLayoutBuilder builder;
        auto& write_descriptor_set = write_descriptor_sets[set];

        for (auto& [binding_index, uniform_buffer] : shader_descriptor_set.uniform_buffers)
        {
            builder.add_binding(binding_index, vk::DescriptorType::eUniformBuffer, to_shader_stage(uniform_buffer.stage), 1);
            write_descriptor_set[uniform_buffer.name] = {
                .dstBinding = static_cast<uint32_t>(binding_index),
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
            };
        }

        for (auto& [binding_index, storage_buffer] : shader_descriptor_set.storage_buffers)
        {
            builder.add_binding(binding_index, vk::DescriptorType::eStorageBuffer, to_shader_stage(storage_buffer.stage), 1);
            PORTAL_ASSERT(!shader_descriptor_set.uniform_buffers.contains(binding_index), "Binding is already present!");

            write_descriptor_set[storage_buffer.name] = {
                .dstBinding = static_cast<uint32_t>(binding_index),
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
            };
        }

        for (auto& [binding_index, image_sampler] : shader_descriptor_set.image_samplers)
        {
            builder.add_binding(
                binding_index,
                vk::DescriptorType::eCombinedImageSampler,
                to_shader_stage(image_sampler.stage),
                static_cast<uint32_t>(image_sampler.array_size)
                );
            PORTAL_ASSERT(!shader_descriptor_set.uniform_buffers.contains(binding_index), "Binding is already present!");
            PORTAL_ASSERT(!shader_descriptor_set.storage_buffers.contains(binding_index), "Binding is already present!");

            write_descriptor_set[image_sampler.name] = {
                .dstBinding = static_cast<uint32_t>(binding_index),
                .descriptorCount = static_cast<uint32_t>(image_sampler.array_size),
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            };
        }

        for (auto& [binding_index, image] : shader_descriptor_set.images)
        {
            builder.add_binding(
                binding_index,
                vk::DescriptorType::eSampledImage,
                to_shader_stage(image.stage),
                static_cast<uint32_t>(image.array_size)
                );
            PORTAL_ASSERT(!shader_descriptor_set.uniform_buffers.contains(binding_index), "Binding is already present!");
            PORTAL_ASSERT(!shader_descriptor_set.storage_buffers.contains(binding_index), "Binding is already present!");
            PORTAL_ASSERT(!shader_descriptor_set.image_samplers.contains(binding_index), "Binding is already present!");

            write_descriptor_set[image.name] = {
                .dstBinding = static_cast<uint32_t>(binding_index),
                .descriptorCount = static_cast<uint32_t>(image.array_size),
                .descriptorType = vk::DescriptorType::eSampledImage,
            };
        }

        for (auto& [binding_index, sampler] : shader_descriptor_set.samplers)
        {
            builder.add_binding(
                binding_index,
                vk::DescriptorType::eSampler,
                to_shader_stage(sampler.stage),
                static_cast<uint32_t>(sampler.array_size)
                );
            PORTAL_ASSERT(!shader_descriptor_set.uniform_buffers.contains(binding_index), "Binding is already present!");
            PORTAL_ASSERT(!shader_descriptor_set.storage_buffers.contains(binding_index), "Binding is already present!");
            PORTAL_ASSERT(!shader_descriptor_set.image_samplers.contains(binding_index), "Binding is already present!");
            PORTAL_ASSERT(!shader_descriptor_set.images.contains(binding_index), "Binding is already present!");

            write_descriptor_set[sampler.name] = {
                .dstBinding = static_cast<uint32_t>(binding_index),
                .descriptorCount = static_cast<uint32_t>(sampler.array_size),
                .descriptorType = vk::DescriptorType::eSampler,
            };
        }

        for (auto& [binding_index, storage_image] : shader_descriptor_set.storage_images)
        {
            builder.add_binding(
                binding_index,
                vk::DescriptorType::eStorageImage,
                to_shader_stage(storage_image.stage),
                static_cast<uint32_t>(storage_image.array_size)
                );
            PORTAL_ASSERT(!shader_descriptor_set.uniform_buffers.contains(binding_index), "Binding is already present!");
            PORTAL_ASSERT(!shader_descriptor_set.storage_buffers.contains(binding_index), "Binding is already present!");
            PORTAL_ASSERT(!shader_descriptor_set.image_samplers.contains(binding_index), "Binding is already present!");
            PORTAL_ASSERT(!shader_descriptor_set.images.contains(binding_index), "Binding is already present!");
            PORTAL_ASSERT(!shader_descriptor_set.samplers.contains(binding_index), "Binding is already present!");

            write_descriptor_set[storage_image.name] = {
                .dstBinding = static_cast<uint32_t>(binding_index),
                .descriptorCount = static_cast<uint32_t>(storage_image.array_size),
                .descriptorType = vk::DescriptorType::eStorageImage,
            };
        }

        LOGGER_TRACE(
            "Creating descriptor set {} with {} ubo's {} ssbo's, {} image samplers, {} images, {} samplers, {} storage images",
            set,
            shader_descriptor_set.uniform_buffers.size(),
            shader_descriptor_set.storage_buffers.size(),
            shader_descriptor_set.image_samplers.size(),
            shader_descriptor_set.images.size(),
            shader_descriptor_set.samplers.size(),
            shader_descriptor_set.storage_images.size()
            );

        builder.set_name(STRING_ID(fmt::format("{}_layout_{}", name.string, set)));
        descriptor_layouts.emplace_back(device->create_descriptor_set_layout(builder));
    }
}


} // portal
