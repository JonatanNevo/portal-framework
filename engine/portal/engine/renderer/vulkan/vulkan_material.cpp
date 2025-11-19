//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_material.h"

#include <ranges>

#include "portal/core/string_utils.h"

#include "portal/engine/renderer/image/texture.h"
#include "portal/engine/renderer/image/image.h"

#include "portal/engine/renderer/descriptors/descriptor_set_manager.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/vulkan_shader.h"
#include "portal/engine/renderer/vulkan/vulkan_render_target.h"
#include "portal/engine/renderer/vulkan/descriptors/vulkan_storage_buffer.h"

namespace portal::renderer::vulkan
{

VulkanMaterial::VulkanMaterial(const MaterialProperties& properties, const VulkanContext& context): Material(properties.id), properties(properties), device(context.get_device())
{
    shader_variant = reference_cast<VulkanShaderVariant, ShaderVariant>(properties.shader);

    // TODO: register dependency on shader

    const DescriptorSetManagerProperties descriptor_prop{
        .shader = shader_variant,
        .debug_name = id,
        .start_set = properties.set_start_index,
        .end_set = properties.set_end_index,
        .default_texture = properties.default_texture,
        .frame_in_flights = 3 // TODO: get it from somewhere?
    };
    descriptor_manager = VulkanDescriptorSetManager::create_unique(descriptor_prop, device);

    for (const auto& [name, decl] : descriptor_manager->input_declarations)
    {
        switch (decl.type)
        {
        case DescriptorType::CombinedImageSampler:
        {
            descriptor_manager->set_input(name, properties.default_texture);
            break;
        }
        default:
            break;
        }
    }

    allocate_storage();

    PORTAL_ASSERT(descriptor_manager->validate(), "Failed to validate descriptor manager");
    descriptor_manager->bake();
}

VulkanMaterial::~VulkanMaterial()
{
    buffers.clear();
}

void VulkanMaterial::set_pipeline(const Reference<VulkanPipeline>& new_pipeline)
{
    pipeline = new_pipeline;
}

Reference<VulkanPipeline> VulkanMaterial::get_pipeline() const
{
    return pipeline;
}

void VulkanMaterial::set(const StringId bind_point, const ResourceReference<Texture>& texture)
{
    set(bind_point, texture.underlying());
}

void VulkanMaterial::set(const StringId bind_point, const Reference<Texture>& texture)
{
    descriptor_manager->set_input(bind_point, texture);
}

void VulkanMaterial::set(const StringId bind_point, const Reference<Image>& image)
{
    descriptor_manager->set_input(bind_point, image);
}

void VulkanMaterial::set(const StringId bind_point, const Reference<ImageView>& image)
{
    descriptor_manager->set_input(bind_point, image);
}

Reference<Texture> VulkanMaterial::get_texture(const StringId bind_point)
{
    return descriptor_manager->get_input<portal::renderer::Texture>(bind_point);
}

Reference<Image> VulkanMaterial::get_image(const StringId bind_point)
{
    return descriptor_manager->get_input<portal::renderer::Image>(bind_point);
}

Reference<ImageView> VulkanMaterial::get_image_view(const StringId bind_point)
{
    return descriptor_manager->get_input<portal::renderer::ImageView>(bind_point);
}

Reference<ShaderVariant> VulkanMaterial::get_shader()
{
    return shader_variant;
}

vk::DescriptorSet VulkanMaterial::get_descriptor_set(const size_t index) const
{
    if (descriptor_manager->get_first_set_index() == std::numeric_limits<size_t>::max())
        return nullptr;

    descriptor_manager->invalidate_and_update(index);
    return descriptor_manager->get_descriptor_sets(index)[0];
}

bool VulkanMaterial::operator==(const VulkanMaterial& other) const
{
    return id == other.id;
}


void VulkanMaterial::set_property(StringId bind_point, const reflection::Property& property)
{
    if (!uniforms.contains(bind_point))
    {
        LOG_ERROR("VulkanMaterial::set_property: bind point {} not found", bind_point);
        return;
    }

    const auto uniform_ptr = uniforms.at(bind_point);
    PORTAL_ASSERT(
        property.type == uniform_ptr.uniform.property.type,
        "Mismatching uniform types - expected: {}, given: {}",
        uniform_ptr.uniform.property.type,
        property.type
        );
    PORTAL_ASSERT(
        property.elements_number == uniform_ptr.uniform.property.elements_number,
        "Mismatching uniform element number - expected: {}, given: {}",
        uniform_ptr.uniform.property.elements_number,
        property.elements_number
        );
    PORTAL_ASSERT(
        property.container_type == uniform_ptr.uniform.property.container_type,
        "Mismatching uniform container type - expected: {}, given: {}",
        uniform_ptr.uniform.property.container_type,
        property.container_type
        );

    auto& buffer = buffers.at(uniform_ptr.buffer_name);
    buffer->set_data(property.value, uniform_ptr.uniform.offset);
}

bool VulkanMaterial::get_property(StringId bind_point, reflection::Property& property) const
{
    if (!uniforms.contains(bind_point))
    {
        LOG_ERROR("VulkanMaterial::get_property: bind point {} not found", bind_point);
        return false;
    }

    const auto uniform_ptr = uniforms.at(bind_point);
    auto& buffer = buffers.at(uniform_ptr.buffer_name);
    const auto& buffer_local_storage = buffer->get_data();

    property = uniform_ptr.uniform.property;
    property.value = Buffer{buffer_local_storage.as<const uint8_t*>() + uniform_ptr.uniform.offset, uniform_ptr.uniform.size};
    return true;
}

template <typename T>
std::unordered_map<StringId, VulkanMaterial::UniformPointer> get_uniform_pointer(T& buffer)
{
    std::unordered_map<StringId, VulkanMaterial::UniformPointer> output;
    for (auto& [uniform_name, uniform] : buffer.uniforms)
    {
        output[uniform_name] = VulkanMaterial::UniformPointer{
            .bind_point = uniform_name,
            .buffer_name = buffer.name,
            .uniform = uniform,
        };
    }

    return output;
}

void VulkanMaterial::allocate_storage()
{
    for (const auto& resource : shader_variant->get_shader_resources() | std::views::values)
    {
        std::unordered_map<StringId, UniformPointer> buffer_uniforms;

        // TODO: skip if input is out of scope
        if (resource.type == DescriptorType::UniformBuffer)
        {
            auto& data = shader_variant->get_reflection().descriptor_sets[resource.set].uniform_buffers.at(resource.binding_index);
            buffer_uniforms = get_uniform_pointer(data);

            auto buffer = make_reference<VulkanUniformBuffer>(data.size, device);
            buffers[resource.name] = reference_cast<BufferDescriptor>(buffer);
            descriptor_manager->set_input(data.name, reference_cast<UniformBuffer>(buffer));
        }

        if (resource.type == DescriptorType::StorageBuffer)
        {
            auto& data = shader_variant->get_reflection().descriptor_sets[resource.set].storage_buffers.at(resource.binding_index);
            buffer_uniforms = get_uniform_pointer(data);

            StorageBufferProperties buf_prop{
                .size = data.size,
                .gpu_only = false,
                .debug_name = STRING_ID("storage buffer")
            };

            auto buffer = make_reference<VulkanStorageBuffer>(buf_prop, device);
            buffers[resource.name] = reference_cast<BufferDescriptor>(buffer);
            descriptor_manager->set_input(data.name, reference_cast<StorageBuffer>(buffer));
        }

        if (!buffer_uniforms.empty())
        {
            uniforms.merge(buffer_uniforms);
        }
    }
}
} // portal
