//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <cstdint>

namespace portal::renderer
{
/**
 * @enum DescriptorType
 * @brief Vulkan descriptor types for shader bindings
 *
 * Maps to vk::DescriptorType. Includes buffers, images, samplers, and acceleration structures.
 */
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

/**
 * @enum DescriptorResourceType
 * @brief High-level descriptor resource categories
 *
 * Categorizes renderer resources for descriptor binding (buffers, textures, images).
 */
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
