//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/buffer.h"
#include "portal/core/reflection/concepts.h"
#include "portal/engine/renderer/allocated_buffer.h"
#include "portal/engine/renderer/allocated_image.h"
#include "portal/engine/renderer/descriptor_writer.h"
#include "portal/engine/resources/resources/resource.h"
#include "portal/engine/shaders/shader_types.h"

namespace portal::resources
{
class GpuContext;
}

namespace portal
{
class Texture;

namespace resources
{
    class ShaderLoader;
}

namespace vulkan
{
    class VulkanShader;
}

class Shader final : public Resource
{
public:
    explicit Shader(const StringId& id) : Resource(id) {}
    void copy_from(Ref<Resource> other) override;

    const std::string& get_entry_point(ShaderStage stage) const;

    template <typename T> requires std::integral<std::remove_const_t<T>> || std::floating_point<std::remove_const_t<T>>
    void bind(const StringId bind_point, const T& t)
    {
        bind_property(
            bind_point,
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(&t)), sizeof(T)},
                reflection::get_property_type<std::remove_const_t<T>>(),
                reflection::PropertyContainerType::scalar,
                1
            }
            );
    }

    template <reflection::IsVec T>
    void bind(const StringId bind_point, const T& t)
    {
        bind_property(
           bind_point,
           reflection::Property{
               Buffer{&t, sizeof(T)},
               reflection::get_property_type<typename T::value_type>(),
               reflection::PropertyContainerType::vector,
               T::length()
           }
           );
    }

    template<reflection::IsMatrix T>
    void bind(const StringId bind_point, const T& t)
    {
        bind_property(
           bind_point,
           reflection::Property{
               Buffer{&t, sizeof(T)},
               reflection::get_property_type<typename T::value_type>(),
               reflection::PropertyContainerType::vector,
               T::length() * T::length()
           }
           );
    }

    void bind_property(StringId bind_point, const reflection::Property& property) const;
    void bind_texture(StringId bind_point, Ref<Texture> texture);

    [[nodiscard]] bool check_all_bind_points_occupied() const;

protected:
    struct ImageBinding
    {
        StringId name;

        size_t set_index;
        vk::DescriptorSetLayoutBinding binding;
        vulkan::AllocatedImage* image = nullptr;
        vk::raii::Sampler* sampler = nullptr;
        bool bound = false;

        [[nodiscard]] bool is_bound() const { return bound; }
    };

    struct BufferField
    {
        FieldLayout layout;
        bool bound = false;
    };

    struct BufferBinding
    {
        StringId name;

        size_t set_index;
        vk::DescriptorSetLayoutBinding binding;
        vulkan::AllocatedBuffer buffer = nullptr;
        Buffer buffer_view;

        std::unordered_map<StringId, BufferField> fields;
        bool bound = false;
        bool external = false;

        [[nodiscard]] bool is_bound() const;
    };

    struct BindingPointer
    {
        union
        {
            BufferBinding* buffer;
            ImageBinding* image;
        };

        StringId field_name = INVALID_STRING_ID;
    };

    void set_shader_reflection(const ShaderReflection& new_reflection, const resources::GpuContext* context);
    BufferBinding& setup_buffer_binding(size_t set_index, const ShaderDescriptorBinding& description, const resources::GpuContext* context);
    ImageBinding& setup_image_binding(size_t set_index, const ShaderDescriptorBinding& description);

protected:
    friend class resources::ShaderLoader;
    friend class vulkan::VulkanShader;
    friend class resources::GpuContext;

    std::vector<vulkan::DescriptorWriter> descriptor_writers;

    std::vector<BufferBinding> buffer_bindings;
    std::vector<ImageBinding> image_bindings;
    std::unordered_map<StringId, BindingPointer> binding_points;

    ShaderReflection reflection = {};
    Buffer code = nullptr;
};

} // portal
