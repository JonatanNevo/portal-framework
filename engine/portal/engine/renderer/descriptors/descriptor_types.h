//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <cstdint>

namespace portal::renderer
{

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

enum class DescriptorResourceType: uint8_t
{
    Unknown,
    UniformBuffer,
    UniformBufferSet,
    StorageBuffer,
    StorageBufferSet,
    Texture,
    TextureCube,
    Image
};
}

namespace portal::utils
{

inline std::string to_string(const renderer::DescriptorResourceType type)
{
    switch (type)
    {
    case renderer::DescriptorResourceType::Unknown:
        return "Unknown";
    case renderer::DescriptorResourceType::UniformBuffer:
        return "UniformBuffer";
    case renderer::DescriptorResourceType::UniformBufferSet:
        return "UniformBufferSet";
    case renderer::DescriptorResourceType::StorageBuffer:
        return "StorageBuffer";
    case renderer::DescriptorResourceType::StorageBufferSet:
        return "StorageBufferSet";
    case renderer::DescriptorResourceType::Texture:
        return "Texture";
    case renderer::DescriptorResourceType::TextureCube:
        return "TextureCube";
    case renderer::DescriptorResourceType::Image:
        return "Image";
    }
    return "Unknown";
}
}

