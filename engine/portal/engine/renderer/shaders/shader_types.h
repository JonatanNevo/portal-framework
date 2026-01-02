//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/reflection/property.h>

#include "portal/engine/renderer/descriptors/descriptor_types.h"
#include "portal/core/strings/string_id.h"

namespace portal::renderer
{
/**
 * @struct ShaderDefine
 * @brief Preprocessor define for shader compilation
 */
struct ShaderDefine
{
    std::string name;
    std::string value;
};

/**
 * @enum ShaderStage
 * @brief Shader pipeline stages
 *
 * Supports graphics (vertex, fragment, geometry, mesh), compute, and raytracing stages.
 */
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

/**
 * @namespace shader_reflection
 * @brief Shader reflection metadata extracted during compilation
 */
namespace shader_reflection
{
    /**
     * @struct ShaderResourceDeclaration
     * @brief Descriptor resource metadata (set, binding, type)
     */
    struct ShaderResourceDeclaration
    {
        StringId name;
        DescriptorType type;
        size_t set{};
        size_t binding_index{};
        size_t count{};
    };

    /**
     * @struct Uniform
     * @brief Uniform variable metadata (offset, size, type)
     */
    struct Uniform
    {
        StringId name;
        reflection::Property property;
        size_t size{};
        size_t offset{};
    };

    /**
     * @struct BufferDescriptor
     * @brief Uniform or storage buffer descriptor metadata
     */
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

    /**
     * @struct ImageSamplerDescriptor
     * @brief Image, sampler, or combined image-sampler descriptor metadata
     */
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

    /**
     * @struct PushConstantsRange
     * @brief Push constant range metadata (stage, offset, size)
     */
    struct PushConstantsRange
    {
        ShaderStage stage;
        size_t offset;
        size_t size;
    };

    /**
     * @struct ShaderDescriptorSet
     * @brief Aggregates all descriptors in a single descriptor set
     */
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

    /**
     * @struct StageInfo
     * @brief Shader stage and entry point name
     */
    struct StageInfo
    {
        ShaderStage stage;
        std::string entry_point;
    };
}

/**
 * @struct ShaderReflection
 * @brief Complete shader reflection data
 *
 * Contains descriptor sets, resource declarations, push constants, and stage info
 * extracted during shader compilation.
 */
struct ShaderReflection
{
    std::vector<shader_reflection::ShaderDescriptorSet> descriptor_sets;
    std::unordered_map<StringId, shader_reflection::ShaderResourceDeclaration> resources;
    std::vector<shader_reflection::PushConstantsRange> push_constants;
    std::vector<shader_reflection::StageInfo> stages;
};
}
