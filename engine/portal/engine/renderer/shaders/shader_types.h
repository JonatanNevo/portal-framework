//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/reflection/property.h>

#include "portal/engine/strings/string_id.h"

namespace portal::renderer
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

namespace shader_reflection
{

    struct ShaderResourceDeclaration
    {
        StringId name;
        size_t set;
        size_t binding_index;
        size_t count;
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
    };

    struct ImageSampler: ImageSamplerDescriptor
    {
    };

    struct Image: ImageSamplerDescriptor
    {
    };

    struct StorageImage: ImageSamplerDescriptor
    {
    };

    struct Sampler: ImageSamplerDescriptor
    {
    };

    struct PushConstantsRange
    {
        ShaderStage stage;
        size_t offset;
        size_t size;
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
    std::unordered_map<StringId, shader_reflection::ShaderResourceDeclaration> Resources;
    std::vector<shader_reflection::PushConstantsRange> push_constants;
};

}
