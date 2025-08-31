//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/reflection/property.h>

#include "portal/engine/strings/string_id.h"

namespace portal::renderer
{

struct ShaderUniform
{
    StringId name;
    reflection::Property property;
    size_t size{};
    size_t offset{};

    void serialize(Serializer& s) const;
    static ShaderUniform deserialize(Deserializer& d);
};

struct ShaderBuffer
{
    StringId name;
    size_t size;
    std::unordered_map<StringId, ShaderUniform> uniforms;

    void serialize(Serializer& s) const;
    static ShaderBuffer deserialize(Deserializer& d);
};

struct ShaderResourceDeclaration
{
    StringId name;
    size_t set;
    size_t binding_index;
    size_t count;

    void serialize(Serializer& s) const;
    static ShaderResourceDeclaration deserialize(Deserializer& d);
};

enum class ShaderStage: uint8_t
{
    All,
    Vertex,
    Fragment,
    Geometry,
    Compute,
    RayGeneration,
    Intersection,
    AnyHit,
    ClosestHit,
    Miss,
    Callable,
    Mesh
};

enum class DescriptorType : uint8_t
{
    Unknown,
    Sampler,
    CombinedImageSampler,
    SampledImage,
    StorageImage,
    UniformTexelBuffer,
    StorageTexelBuffer,
    UniformBuffer,
    StorageBuffer,
    UniformBufferDynamic,
    StorageBufferDynamic,
    InputAttachment,
    AccelerationStructure,
    InlineUniformBlock,
};

namespace shader_reflection
{
    struct BufferDescriptor
    {
        DescriptorType type;
        ShaderStage stage;
        size_t size;
        size_t offset;
        size_t range;
        size_t binding_point;
        StringId name;

        void serialize(Serializer& s) const;
        static BufferDescriptor deserialize(Deserializer& d);
    };

    struct UniformBuffer: BufferDescriptor
    {
        DescriptorType type = DescriptorType::UniformBuffer;
    };

    struct StorageBuffer: BufferDescriptor
    {
        DescriptorType type = DescriptorType::StorageBuffer;
    };

    struct ImageSamplerDescriptor
    {
        DescriptorType type;
        ShaderStage stage;
        size_t binding_point;
        size_t descriptor_set;
        size_t dimensions;
        size_t array_size;
        StringId name;

        void serialize(Serializer& s) const;
        static ImageSamplerDescriptor deserialize(Deserializer& d);
    };

    struct ImageSampler: ImageSamplerDescriptor
    {
        DescriptorType type = DescriptorType::CombinedImageSampler;
    };

    struct Image: ImageSamplerDescriptor
    {
        DescriptorType type = DescriptorType::SampledImage;
    };

    struct StorageImage: ImageSamplerDescriptor
    {
        DescriptorType type = DescriptorType::StorageImage;
    };

    struct Sampler: ImageSamplerDescriptor
    {
        DescriptorType type = DescriptorType::Sampler;
    };

    struct PushConstantsRange
    {
        ShaderStage stage;
        size_t offset;
        size_t size;

        void serialize(Serializer& s) const;
        static PushConstantsRange deserialize(Deserializer& d);
    };

    struct ShaderDescriptorSet
    {
        std::unordered_map<uint32_t, UniformBuffer> uniform_buffers;
        std::unordered_map<uint32_t, StorageBuffer> storage_buffers;
        std::unordered_map<uint32_t, ImageSampler> image_samplers;
        std::unordered_map<uint32_t, StorageImage> storage_images;
        std::unordered_map<uint32_t, Image> images;
        std::unordered_map<uint32_t, Sampler> samplers;
    };
}

struct ShaderReflection
{
    std::vector<shader_reflection::ShaderDescriptorSet> descriptor_sets;
    std::unordered_map<StringId, ShaderResourceDeclaration> Resources;
    std::unordered_map<StringId, ShaderBuffer> constant_buffers;
    std::vector<shader_reflection::PushConstantsRange> push_constants;
};

// struct ShaderBufferElement
// {
//     StringId name;
//     // Value is ignored, only looking at the reflection
//     reflection::Property property;
//     size_t size;
//     size_t offset;
//
//     bool normalized = false;
// };
//
// struct ShaderDescriptorBinding
// {
//     ShaderStage stage;
//     size_t binding_index;
//     DescriptorType type;
//     size_t descriptor_count;
//     StringId name = INVALID_STRING_ID;
//
//     // Only relevant in buffer types
//     std::unordered_map<StringId, ShaderBufferElement> fields;
// };
//
// struct DescriptorBindingPointer
// {
//     StringId name{};
//     size_t set_index = std::numeric_limits<size_t>::max();
//     size_t binding_index = std::numeric_limits<size_t>::max();
//     std::optional<StringId> field_name = std::nullopt;
// };
//
// struct ShaderDescriptorLayout
// {
//     StringId name;
//     std::vector<ShaderDescriptorBinding> bindings;
// };
//
// struct ShaderPushConstant
// {
//     StringId name;
//     ShaderStage stage = ShaderStage::All;
//     size_t size{};
//     size_t offset{};
// };
//
// struct ShaderReflection
// {
//     std::unordered_map<ShaderStage, std::string> entry_points;
//     std::vector<ShaderDescriptorLayout> layouts;
//     std::vector<ShaderPushConstant> push_constants;
//     std::unordered_map<StringId, DescriptorBindingPointer> bind_points;
// };
}

namespace portal::utils
{
    std::string to_string(renderer::DescriptorType type);
}

template <>
struct fmt::formatter<portal::renderer::DescriptorType>
{
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const portal::renderer::DescriptorType& type, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", portal::utils::to_string(type));
    }
};