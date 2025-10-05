//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/reflection/property.h>

#include "portal/engine/renderer/descriptors/descriptor_types.h"
#include "portal/engine/strings/string_id.h"

namespace portal::renderer
{

struct ShaderDefine
{
    std::string name;
    std::string value;
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

namespace shader_reflection
{

    struct ShaderResourceDeclaration
    {
        StringId name;
        DescriptorType type;
        size_t set{};
        size_t binding_index{};
        size_t count{};
    };

    struct Uniform
    {
        StringId name;
        reflection::Property property;
        size_t size{};
        size_t offset{};
    };

    struct BufferDescriptor
    {
        DescriptorType type;
        ShaderStage stage;
        size_t size;
        size_t offset;
        size_t range;
        size_t binding_point;
        StringId name;

        std::unordered_map<StringId, Uniform> uniforms;
    };

    struct ImageSamplerDescriptor
    {
        DescriptorType type = DescriptorType::Unknown;
        ShaderStage stage = ShaderStage::All;
        size_t binding_point{};
        size_t descriptor_set{};
        size_t dimensions{};
        size_t array_size{};
        StringId name;
    };

    struct PushConstantsRange
    {
        ShaderStage stage;
        size_t offset;
        size_t size;
    };

    struct ShaderDescriptorSet
    {
        std::unordered_map<size_t, BufferDescriptor> uniform_buffers;
        std::unordered_map<size_t, BufferDescriptor> storage_buffers;
        std::unordered_map<size_t, ImageSamplerDescriptor> image_samplers;
        std::unordered_map<size_t, ImageSamplerDescriptor> storage_images;
        std::unordered_map<size_t, ImageSamplerDescriptor> images;
        std::unordered_map<size_t, ImageSamplerDescriptor> samplers;

        explicit operator bool() const
        {
            return !(uniform_buffers.empty() && storage_buffers.empty() && image_samplers.empty() && storage_images.empty() && images.
                empty() && samplers.empty());
        }

    };

    struct StageInfo
    {
        ShaderStage stage;
        std::string entry_point;
    };
}

struct ShaderReflection
{
    std::vector<shader_reflection::ShaderDescriptorSet> descriptor_sets;
    std::unordered_map<StringId, shader_reflection::ShaderResourceDeclaration> resources;
    std::vector<shader_reflection::PushConstantsRange> push_constants;
    std::vector<shader_reflection::StageInfo> stages;
};

}

namespace portal::utils
{

std::string to_string(const renderer::DescriptorType type);

}
