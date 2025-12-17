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
