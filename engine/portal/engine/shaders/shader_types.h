//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/reflection/property.h>

#include "portal/engine/strings/string_id.h"

namespace portal
{

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

struct DescriptorLayout
{
    StringId name;
    // Value is ignored, only looking at the reflection
    reflection::Property property;
    size_t offset;
    size_t size;
};

struct ShaderDescriptorBinding
{
    ShaderStage stage;
    size_t binding_index;
    DescriptorType type;
    size_t descriptor_count;
    StringId name = INVALID_STRING_ID;

    // Only relevant in buffer types
    std::unordered_map<StringId, DescriptorLayout> layout;
};

struct DescriptorBindingPointer
{
    StringId name{};
    size_t layout_index = std::numeric_limits<size_t>::max();
    size_t binding_index = std::numeric_limits<size_t>::max();
    std::optional<StringId> layout_name = std::nullopt;
};

struct ShaderDescriptorLayout
{
    StringId name;
    std::vector<ShaderDescriptorBinding> bindings;
};

struct ShaderPushConstant
{
    StringId name;
    ShaderStage stage;
    size_t size{};
    size_t offset{};
};

struct ShaderReflection
{
    std::unordered_map<ShaderStage, std::string> entry_points;
    std::vector<ShaderDescriptorLayout> layouts;
    std::vector<ShaderPushConstant> push_constants;
    std::unordered_map<StringId, DescriptorBindingPointer> bind_points;
};

namespace utils
{
    std::string to_string(DescriptorType type);
}
}

template <>
struct fmt::formatter<portal::DescriptorType>
{
    static constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const portal::DescriptorType& type, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", portal::utils::to_string(type));
    }
};