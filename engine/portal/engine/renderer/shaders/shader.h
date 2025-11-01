//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <filesystem>

#include "portal/core/buffer.h"
#include "portal/core/reflection/concepts.h"
#include "../vulkan/image/vulkan_image.h"
#include "portal/engine/reference.h"
#include "portal/engine/renderer/shaders/shader_types.h"
#include "portal/engine/renderer/descriptor_writer.h"

namespace portal
{
class Deserializer;
class Serializer;
}

namespace portal::renderer
{
struct CompiledShader;

namespace vulkan
{
    class VulkanImage;
    class VulkanShaderVariant;
}


class ShaderVariant
{
public:
    virtual ~ShaderVariant() = default;

    [[nodiscard]] virtual StringId get_name() const = 0;

    virtual const std::unordered_map<StringId, shader_reflection::ShaderResourceDeclaration>& get_shader_resources() const = 0;
    virtual const ShaderReflection& get_reflection() const = 0;
};

/**
 * Generates and caches `ShaderVariant` resource for multiple materials per shader.
 */
class Shader : public Resource
{
public:
    DECLARE_RESOURCE(ResourceType::Shader);

    explicit Shader(const StringId& id);

    void load_source(Buffer&& new_source, const std::filesystem::path& shader_path);

    /**
     * Compiles the shader with a given list of permutations (defines)
     *
     * @param permutations A list of permutations (name, value)
     * @return The hash to fetch the shader code with.
     */
    uint64_t compile_with_permutations(const std::vector<ShaderDefine>& permutations);

    virtual WeakReference<ShaderVariant> get_shader(uint64_t shader_hash) = 0;

protected:
    uint64_t calculate_permutations_hash(const std::vector<ShaderDefine>& permutations) const;

protected:
    std::filesystem::path source_path;
    Buffer source;
    std::unordered_map<uint64_t, CompiledShader> shaders;
};

// namespace details
// {
//
//     struct ImageBinding
//     {
//         StringId name;
//
//         size_t set_index{};
//         vk::DescriptorSetLayoutBinding binding;
//         renderer::vulkan::VulkanImage* image = nullptr;
//         vk::raii::Sampler* sampler = nullptr;
//         bool bound = false;
//
//         [[nodiscard]] bool is_bound() const { return bound; }
//     };
//
//     struct BufferField
//     {
//         ShaderBufferElement layout;
//         bool bound = false;
//     };
//
//     struct BufferBinding
//     {
//         StringId name;
//
//         size_t set_index;
//         vk::DescriptorSetLayoutBinding binding;
//         size_t size;
//         renderer::vulkan::AllocatedBuffer* buffer = nullptr;
//         Buffer buffer_view;
//         std::unordered_map<StringId, BufferField> fields;
//         bool bound = false;
//
//         [[nodiscard]] bool is_bound() const;
//     };
//
//     struct BindingPointer
//     {
//         BufferBinding* buffer = nullptr;
//         ImageBinding* image = nullptr;
//
//         StringId field_name = INVALID_STRING_ID;
//     };
//
//     struct DescriptorSetBinding
//     {
//         bool global = false;
//         std::vector<BindingPointer> bindings;
//     };
//
//     class ShaderBindingContext
//     {
//     public:
//         explicit ShaderBindingContext(const Ref<Shader>& shader);
//         ~ShaderBindingContext();
//
//         template <typename T> requires std::integral<std::remove_const_t<T>> || std::floating_point<std::remove_const_t<T>>
//         void bind(const StringId bind_point, const T& t)
//         {
//             bind_property(
//                 bind_point,
//                 reflection::Property{
//                     Buffer{const_cast<void*>(static_cast<const void*>(&t)), sizeof(T)},
//                     reflection::get_property_type<std::remove_const_t<T>>(),
//                     reflection::PropertyContainerType::scalar,
//                     1
//                 }
//                 );
//         }
//
//         template <reflection::IsVec T>
//         void bind(const StringId bind_point, const T& t)
//         {
//             bind_property(
//                 bind_point,
//                 reflection::Property{
//                     Buffer{&t, sizeof(T)},
//                     reflection::get_property_type<typename T::value_type>(),
//                     reflection::PropertyContainerType::vector,
//                     T::length()
//                 }
//                 );
//         }
//
//         template <reflection::IsMatrix T>
//         void bind(const StringId bind_point, const T& t)
//         {
//             bind_property(
//                 bind_point,
//                 reflection::Property{
//                     Buffer{&t, sizeof(T)},
//                     reflection::get_property_type<typename T::value_type>(),
//                     reflection::PropertyContainerType::vector,
//                     T::length() * T::length()
//                 }
//                 );
//         }
//
//         void bind_property(StringId bind_point, const reflection::Property& property) const;
//         void bind_texture(StringId bind_point, Ref<Texture> texture);
//         void bind_buffer(StringId bind_point, vulkan::AllocatedBuffer* buffer);
//
//         void mark_set_as_global(size_t index);
//
//         void write_to_sets(std::vector<vk::raii::DescriptorSet>& sets, const std::shared_ptr<vulkan::GpuContext>& context);
//
//         [[nodiscard]] std::vector<vulkan::AllocatedBuffer> make_buffers(const std::shared_ptr<vulkan::GpuContext>& context);
//         [[nodiscard]] vulkan::AllocatedBuffer create_and_bind_buffer(StringId name, const std::shared_ptr<vulkan::GpuContext>& context);
//
//         bool check_all_bind_points_occupied() const;
//
//     private:
//         void setup_bindings();
//         BufferBinding& setup_buffer_binding(size_t set_index, const ShaderDescriptorBinding& description);
//         ImageBinding& setup_image_binding(size_t set_index, const ShaderDescriptorBinding& description);
//
//     private:
//         bool writen_to_descriptor_set = false;
//         std::vector<DescriptorSetBinding> descriptor_set_bindings;
//         std::vector<BufferBinding> buffer_bindings;
//         std::vector<ImageBinding> image_bindings;
//
//         std::unordered_map<StringId, BindingPointer> binding_points;
//
//         std::vector<portal::vulkan::DescriptorWriter> descriptor_writers;
//         Ref<Shader> shader;
//     };
// }

} // portal
