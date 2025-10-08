//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "shader.h"

#include <ranges>
#include <nlohmann/detail/input/parser.hpp>

#include "portal/core/reflection/concepts.h"
#include "portal/engine/renderer/shaders/shader_compiler.h"
#include "portal/serialization/serialize.h"

namespace portal::renderer
{

static auto logger = Log::get_logger("Shader");

Shader::Shader(const StringId& id) : Resource(id)
{}

void Shader::load_source(Buffer&& new_source, const std::filesystem::path& shader_path)
{
    source_path = shader_path;
    source = std::move(new_source);
}

uint64_t Shader::compile_with_permutations(const std::vector<ShaderDefine>& permutations)
{
    const auto permutations_hash = calculate_permutations_hash(permutations);

    if (!shaders.contains(permutations_hash))
    {
        LOGGER_DEBUG("Compiling shader variant: {} [{}]", id, permutations_hash);
        ShaderCompiler compiler;
        shaders[permutations_hash] = compiler.compile({.name = id, .shader_path = source_path, .shader_data = source, .defines = permutations});
    }

    return permutations_hash;
}


uint64_t Shader::calculate_permutations_hash(const std::vector<ShaderDefine>& permutations) const
{
    uint64_t hash = id.id;
    for (const auto& [name, value] : permutations)
    {
        hash ^= hash::rapidhash(name);
    }
    return hash;
}

}
//
// namespace portal::renderer
// {
//
// static auto logger = Log::get_logger("Shader");
//
// vk::DescriptorType to_vk_descriptor_type(const DescriptorType type)
// {
// #define CASE(FROM, TO)             \
// case portal::renderer::DescriptorType::FROM: \
// return vk::DescriptorType::TO
//
//     switch (type)
//     {
//     CASE(Sampler, eSampler);
//     CASE(CombinedImageSampler, eCombinedImageSampler);
//     CASE(SampledImage, eSampledImage);
//     CASE(StorageImage, eStorageImage);
//     CASE(UniformTexelBuffer, eUniformTexelBuffer);
//     CASE(StorageTexelBuffer, eStorageTexelBuffer);
//     CASE(UniformBuffer, eUniformBuffer);
//     CASE(StorageBuffer, eStorageBuffer);
//     CASE(UniformBufferDynamic, eUniformBufferDynamic);
//     CASE(StorageBufferDynamic, eStorageBufferDynamic);
//     CASE(InputAttachment, eInputAttachment);
//     CASE(AccelerationStructure, eAccelerationStructureKHR);
//     CASE(InlineUniformBlock, eUniformBufferDynamic);
//     case DescriptorType::Unknown:
//         break;
//     }
// #undef CASE
//
//     LOGGER_ERROR("Unknown descriptor type found");
//     throw std::runtime_error("Unknown descriptor type");
// }
//
//
// details::ShaderBindingContext Shader::start_binding_context()
// {
//     return details::ShaderBindingContext(this);
// }
//
// bool details::BufferBinding::is_bound() const
// {
//     return bound && std::ranges::all_of(fields | std::views::values, [](const BufferField& field) { return field.bound; });
// }
//
// details::ShaderBindingContext::ShaderBindingContext(const Ref<Shader>& shader) : shader(shader)
// {
//     descriptor_writers.resize(shader->source->reflection.layouts.size());
// }
//
// details::ShaderBindingContext::~ShaderBindingContext()
// {
//     if (!writen_to_descriptor_set)
//         LOGGER_ERROR("Closing binding context without writing to descriptor set");
// }
//
// void details::ShaderBindingContext::bind_property(StringId bind_point, const reflection::Property& property) const
// {
//     PORTAL_ASSERT(binding_points.contains(bind_point), "Invalid bind point: {}", bind_point);
//
//     auto& binding_pointer = binding_points.at(bind_point);
//     auto* buffer_binding = binding_pointer.buffer;
//     PORTAL_ASSERT(buffer_binding, "Invalid buffer binding");
//     PORTAL_ASSERT(buffer_binding->is_bound(), "Buffer {} not bound, please bind the corresponding buffer before binding fields", buffer_binding->name);
//
//     auto& [layout, bound] = buffer_binding->fields[binding_pointer.field_name];
//     PORTAL_ASSERT(layout.property == property, "Invalid property");
//
//     buffer_binding->buffer_view.write(property.value.data, property.value.size, layout.offset);
//     if (bound)
//         LOGGER_WARN("Property already bound: {}", bind_point);
//     bound = true;
// }
//
// void details::ShaderBindingContext::bind_texture(StringId bind_point, Ref<Texture> texture)
// {
//     PORTAL_ASSERT(binding_points.contains(bind_point), "Invalid bind point: {}", bind_point);
//     auto* image_binding = binding_points.at(bind_point).image;
//     PORTAL_ASSERT(image_binding, "Invalid image binding");
//
//     image_binding->image = &texture->get_image();
//     image_binding->sampler = &texture->get_sampler();
//     PORTAL_ASSERT(image_binding->image && image_binding->sampler, "Invalid image or sampler");
//
//     if (image_binding->bound)
//         LOGGER_WARN("Property already bound: {}", bind_point);
//
//     descriptor_writers[image_binding->set_index].write_image(
//         image_binding->binding.binding,
//         image_binding->image->get_view(),
//         *image_binding->sampler,
//         vk::ImageLayout::eShaderReadOnlyOptimal,
//         vk::DescriptorType::eCombinedImageSampler
//         );
//
//     image_binding->bound = true;
// }
//
// void details::ShaderBindingContext::bind_buffer(StringId bind_point, renderer::vulkan::AllocatedBuffer* buffer)
// {
//     PORTAL_ASSERT(binding_points.contains(bind_point), "Invalid bind point: {}", bind_point);
//     auto* buffer_binding = binding_points.at(bind_point).buffer;
//     PORTAL_ASSERT(buffer_binding, "Invalid buffer binding");
//
//     buffer_binding->buffer = buffer;
//     buffer_binding->buffer_view = Buffer(buffer->get_data(), buffer->get_size());
//     PORTAL_ASSERT(buffer_binding->buffer && buffer_binding->buffer_view, "Invalid buffer or buffer view");
//
//     if (buffer_binding->bound)
//         LOGGER_WARN("Property already bound: {}", bind_point);
//
//     descriptor_writers[buffer_binding->set_index].write_buffer(
//         buffer_binding->binding.binding,
//         *buffer,
//         buffer_binding->size,
//         0,
//         buffer_binding->binding.descriptorType
//         );
//
//     buffer_binding->bound = true;
// }
//
// void details::ShaderBindingContext::mark_set_as_global(size_t index) {
//     PORTAL_ASSERT(index < descriptor_set_bindings.size(), "Index out of range");
//
//     auto [global, bindings] = descriptor_set_bindings[index];
//     global = true;
//
//     for (const auto& binding : bindings)
//     {
//         if (binding.image != nullptr)
//         {
//             binding.image->bound = true;
//         }
//         else if (binding.buffer != nullptr)
//         {
//             binding.buffer->bound = true;
//             for (auto& [layout, bound] : binding.buffer->fields | std::views::values)
//             {
//                 bound = true;
//             }
//         }
//     }
// }
//
// void details::ShaderBindingContext::write_to_sets(std::vector<vk::raii::DescriptorSet>& sets, const std::shared_ptr<renderer::vulkan::GpuContext>& context)
// {
//     PORTAL_ASSERT(descriptor_set_bindings.size() == sets.size() && sets.size() == descriptor_writers.size(), "size mismatch");
//
//     for (size_t i = 0; i < descriptor_writers.size(); ++i)
//     {
//         auto& descriptor_set_binding = descriptor_set_bindings[i];
//         if (descriptor_set_binding.global)
//             continue;
//
//         context->write_descriptor_set(descriptor_writers[i], sets[i]);
//     }
// }
//
//
// std::vector<renderer::vulkan::AllocatedBuffer> details::ShaderBindingContext::make_buffers(const std::shared_ptr<renderer::vulkan::GpuContext>& context)
// {
//     std::vector<renderer::vulkan::AllocatedBuffer> buffers;
//     buffers.reserve(buffer_bindings.size());
//     for (const auto& buffer_binding : buffer_bindings)
//     {
//         buffers.emplace_back(create_and_bind_buffer(buffer_binding.name, context));
//     }
//     return buffers;
// }
//
// renderer::vulkan::AllocatedBuffer details::ShaderBindingContext::create_and_bind_buffer(StringId name, const std::shared_ptr<renderer::vulkan::GpuContext>& context)
// {
//     PORTAL_ASSERT(binding_points.contains(name), "Invalid bind point: {}", name);
//     auto* buffer_binding = binding_points.at(name).buffer;
//     PORTAL_ASSERT(buffer_binding, "Invalid buffer binding");
//
//     const auto builder = vulkan::BufferBuilder(buffer_binding->size)
//                          .with_usage(vk::BufferUsageFlagBits::eUniformBuffer)
//                          .with_vma_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
//                          .with_vma_flags(VMA_ALLOCATION_CREATE_MAPPED_BIT)
//                          .with_debug_name(fmt::format("{}-buffer-{}", shader->id, buffer_binding->name));
//
//     auto&& buffer = context->create_buffer(builder);
//     bind_buffer(name, &buffer);
//     return buffer;
// }
//
// bool details::ShaderBindingContext::check_all_bind_points_occupied() const
// {
//     const bool image_bounded = std::ranges::all_of(image_bindings, [](const auto& image)
//     {
//         auto bound = image.is_bound();
//         if (!bound)
//         {
//             LOGGER_WARN("Image {} not bound", image.name);
//         }
//         return bound;
//     });
//     const bool buffer_bounded = std::ranges::all_of(buffer_bindings, [](const auto& buffer)
//     {
//         auto bound = buffer.is_bound();
//         if (!bound)
//         {
//             LOGGER_WARN("Buffer {} not bound", buffer.name);
//         }
//         return bound;
//     });
//     return image_bounded && buffer_bounded;
// }
//
// void details::ShaderBindingContext::setup_bindings()
// {
//     binding_points.clear();
//     buffer_bindings.clear();
//     image_bindings.clear();
//
//     binding_points.reserve(shader->source->reflection.bind_points.size());
//     buffer_bindings.reserve(shader->source->reflection.bind_points.size());
//     image_bindings.reserve(shader->source->reflection.bind_points.size());
//
//     descriptor_set_bindings.resize(shader->source->reflection.layouts.size());
//
//     for (auto& [bind_point, indexes] : shader->source->reflection.bind_points)
//     {
//         auto& binding_data = shader->source->reflection.layouts[indexes.set_index].bindings[indexes.binding_index];
//         if (binding_data.type == DescriptorType::UniformBuffer)
//         {
//             // Check if does not exist already
//             BufferBinding* buffer = nullptr;
//             for (auto& binding : buffer_bindings)
//             {
//                 if (binding.name == binding_data.name)
//                 {
//                     buffer = &binding;
//                     break;
//                 }
//             }
//             if (!buffer)
//             {
//                 buffer = &setup_buffer_binding(indexes.set_index, binding_data);
//                 descriptor_set_bindings[indexes.set_index].bindings.push_back({.buffer = buffer });
//                 binding_points[binding_data.name] = {.buffer = buffer };
//             }
//             binding_points[bind_point] = {.buffer = buffer, .field_name = indexes.name};
//         }
//         else if (binding_data.type == DescriptorType::CombinedImageSampler)
//         {
//             auto& image = setup_image_binding(indexes.set_index, binding_data);
//             descriptor_set_bindings[indexes.set_index].bindings.push_back({.image = &image});
//             binding_points[bind_point] = {.image = &image};
//         }
//         else
//             LOGGER_ERROR("Unknown descriptor type found: {}", binding_data.type);
//     }
// }
//
// details::BufferBinding& details::ShaderBindingContext::setup_buffer_binding(const size_t set_index, const ShaderDescriptorBinding& description)
// {
//     const vk::DescriptorSetLayoutBinding binding{
//         .binding = static_cast<uint32_t>(description.binding_index),
//         .descriptorType = to_vk_descriptor_type(description.type),
//         .descriptorCount = static_cast<uint32_t>(description.descriptor_count),
//         .stageFlags = resources::to_vk_shader_stage(description.stage)
//     };
//
//     size_t total_size = 0;
//
//     std::unordered_map<StringId, BufferField> fields;
//     fields.reserve(description.fields.size());
//
//     for (auto& [field_name, field_data] : description.fields)
//     {
//         fields[field_name] = {.layout = field_data, .bound = false};
//         total_size += field_data.size;
//     }
//
//     return buffer_bindings.emplace_back(
//         BufferBinding{
//             .name = description.name,
//             .set_index = set_index,
//             .binding = binding,
//             .size = total_size,
//             .fields = std::move(fields),
//             .bound = false
//         }
//         );
// }
//
// details::ImageBinding& details::ShaderBindingContext::setup_image_binding(size_t set_index, const ShaderDescriptorBinding& description)
// {
//     const vk::DescriptorSetLayoutBinding binding{
//         .binding = static_cast<uint32_t>(description.binding_index),
//         .descriptorType = to_vk_descriptor_type(description.type),
//         .descriptorCount = static_cast<uint32_t>(description.descriptor_count),
//         .stageFlags = resources::to_vk_shader_stage(description.stage)
//     };
//
//     return image_bindings.emplace_back(description.name, set_index, binding, nullptr, nullptr, false);
// }
// } // portal
