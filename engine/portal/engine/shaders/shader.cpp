//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "shader.h"

#include <ranges>

#include "portal/core/reflection/concepts.h"
#include "portal/engine/resources/resources/texture.h"
#include "portal/engine/resources/gpu_context.h"

namespace portal
{

static auto logger = Log::get_logger("Shader");

vk::DescriptorType to_vk_descriptor_type(const DescriptorType type)
{
#define CASE(FROM, TO)             \
case portal::DescriptorType::FROM: \
return vk::DescriptorType::TO

    switch (type)
    {
    CASE(Sampler, eSampler);
    CASE(CombinedImageSampler, eCombinedImageSampler);
    CASE(SampledImage, eSampledImage);
    CASE(StorageImage, eStorageImage);
    CASE(UniformTexelBuffer, eUniformTexelBuffer);
    CASE(StorageTexelBuffer, eStorageTexelBuffer);
    CASE(UniformBuffer, eUniformBuffer);
    CASE(StorageBuffer, eStorageBuffer);
    CASE(UniformBufferDynamic, eUniformBufferDynamic);
    CASE(StorageBufferDynamic, eStorageBufferDynamic);
    CASE(InputAttachment, eInputAttachment);
    CASE(AccelerationStructure, eAccelerationStructureKHR);
    CASE(InlineUniformBlock, eUniformBufferDynamic);
    case DescriptorType::Unknown:
        break;
    }
#undef CASE

    LOGGER_ERROR("Unknown descriptor type found");
    throw std::runtime_error("Unknown descriptor type");
}

bool Shader::BufferBinding::is_bound() const
{
    if (external)
        return bound;

    return bound && std::ranges::all_of(fields | std::views::values, [](const BufferField& field) { return field.bound; });
}

void Shader::copy_from(const Ref<Resource> other)
{
    auto other_shader = other.as<Shader>();
    reflection = other_shader->reflection;
    code = other_shader->code;
}

const std::string& Shader::get_entry_point(const ShaderStage stage) const
{
    return reflection.entry_points.at(stage);
}

void Shader::bind_property(StringId bind_point, const reflection::Property& property) const
{
    PORTAL_ASSERT(binding_points.contains(bind_point), "Invalid bind point: {}", bind_point);

    auto& binding_pointer = binding_points.at(bind_point);
    auto* buffer_binding = binding_pointer.buffer;
    PORTAL_ASSERT(buffer_binding, "Invalid buffer binding");

    auto& field_binding = buffer_binding->fields[binding_pointer.field_name];
    PORTAL_ASSERT(field_binding.layout.property == property, "Invalid property");

    buffer_binding->buffer_view.write(property.value.data, property.value.size, field_binding.layout.offset);
    field_binding.bound = true;
}

void Shader::bind_texture(StringId bind_point, Ref<Texture> texture)
{
    PORTAL_ASSERT(binding_points.contains(bind_point), "Invalid bind point: {}", bind_point);

    auto* image_binding = binding_points.at(bind_point).image;
    PORTAL_ASSERT(image_binding, "Invalid image binding");

    image_binding->image = &texture->get_image();
    image_binding->sampler = &texture->get_sampler();
    PORTAL_ASSERT(image_binding->image && image_binding->sampler, "Invalid image or sampler");

    descriptor_writers[image_binding->set_index].write_image(
        image_binding->binding.binding,
        image_binding->image->get_view(),
        *image_binding->sampler,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::DescriptorType::eCombinedImageSampler
        );

    image_binding->bound = true;
}

bool Shader::check_all_bind_points_occupied() const
{
    const bool image_bounded = std::ranges::all_of(image_bindings, [](const auto& image) { return image.is_bound(); });
    const bool buffer_bounded = std::ranges::all_of(buffer_bindings, [](const auto& buffer) { return buffer.is_bound(); });
    return image_bounded && buffer_bounded;
}

void Shader::set_shader_reflection(const ShaderReflection& new_reflection, const resources::GpuContext* context)
{
    PORTAL_ASSERT(context, "Invalid GPU context");

    reflection = new_reflection;
    binding_points.clear();
    buffer_bindings.clear();
    image_bindings.clear();

    descriptor_writers.clear();
    descriptor_writers.resize(reflection.layouts.size());

    binding_points.reserve(reflection.bind_points.size());
    buffer_bindings.reserve(reflection.bind_points.size());
    image_bindings.reserve(reflection.bind_points.size());


    for (auto& [bind_point, indexes] : reflection.bind_points)
    {
        auto& binding_data = reflection.layouts[indexes.layout_index].bindings[indexes.binding_index];
        if (binding_data.type == DescriptorType::UniformBuffer)
        {
            // Check if does not exist already
            BufferBinding* buffer = nullptr;
            for (auto& binding : buffer_bindings)
            {
                if (binding.name == binding_data.name)
                {
                    buffer = &binding;
                    break;
                }
            }
            if (!buffer)
                buffer = &setup_buffer_binding(indexes.layout_index, binding_data, context);

            binding_points[bind_point] = {.buffer = buffer, .field_name = indexes.name};
        }
        else if (binding_data.type == DescriptorType::CombinedImageSampler)
        {
            auto& image = setup_image_binding(indexes.layout_index, binding_data);
            binding_points[bind_point] = {.image = &image};
        }
        else
            LOGGER_ERROR("Unknown descriptor type found: {}", binding_data.type);

    }
}

Shader::BufferBinding& Shader::setup_buffer_binding(size_t set_index, const ShaderDescriptorBinding& description, const resources::GpuContext* context)
{
    vk::DescriptorSetLayoutBinding binding{
        .binding = static_cast<uint32_t>(description.binding_index),
        .descriptorType = to_vk_descriptor_type(description.type),
        .descriptorCount = static_cast<uint32_t>(description.descriptor_count),
        .stageFlags = resources::to_vk_shader_stage(description.stage)
    };

    size_t total_size = 0;

    std::unordered_map<StringId, BufferField> fields;
    fields.reserve(description.fields.size());

    for (auto& [field_name, field_data] : description.fields)
    {
        fields[field_name] = {.layout = field_data, .bound = false};
        total_size += field_data.size;
    }

    const auto builder = vulkan::BufferBuilder(total_size)
                         .with_usage(vk::BufferUsageFlagBits::eUniformBuffer)
                         .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
                         .with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
                         .with_debug_name(fmt::format("{}-buffer-{}", id.string, description.name));

    auto buffer = context->create_buffer(builder);
    auto buffer_view = Buffer(buffer.get_data(), total_size);

    descriptor_writers[set_index].write_buffer(
        binding.binding,
        buffer,
        total_size,
        0,
        binding.descriptorType
        );
    return buffer_bindings.emplace_back(description.name, set_index, binding, std::move(buffer), buffer_view, std::move(fields), true);
}

Shader::ImageBinding& Shader::setup_image_binding(size_t set_index, const ShaderDescriptorBinding& description)
{
    const vk::DescriptorSetLayoutBinding binding{
        .binding = static_cast<uint32_t>(description.binding_index),
        .descriptorType = to_vk_descriptor_type(description.type),
        .descriptorCount = static_cast<uint32_t>(description.descriptor_count),
        .stageFlags = resources::to_vk_shader_stage(description.stage)
    };

    return image_bindings.emplace_back(description.name, set_index, binding, nullptr, nullptr, false);
}
} // portal
