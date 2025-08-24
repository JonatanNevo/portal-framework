//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "shader_types.h"

namespace portal
{

std::string utils::to_string(DescriptorType type)
{
    switch (type)
    {
    case DescriptorType::Unknown:
        return "Unknown Descriptor Type";
    case DescriptorType::Sampler:
        return "Sampler";
    case DescriptorType::CombinedImageSampler:
        return "CombinedImageSampler";
    case DescriptorType::SampledImage:
        return "SampledImage";
    case DescriptorType::StorageImage:
        return "StorageImage";
    case DescriptorType::UniformTexelBuffer:
        return "UniformTexelBuffer";
    case DescriptorType::StorageTexelBuffer:
        return "StorageTexelBuffer";
    case DescriptorType::UniformBuffer:
        return "UniformBuffer";
    case DescriptorType::StorageBuffer:
        return "StorageBuffer";
    case DescriptorType::UniformBufferDynamic:
        return "UniformBufferDynamic";
    case DescriptorType::StorageBufferDynamic:
        return "StorageBufferDynamic";
    case DescriptorType::InputAttachment:
        return "InputAttachment";
    case DescriptorType::AccelerationStructure:
        return "AccelerationStructure";
    case DescriptorType::InlineUniformBlock:
        return "InlineUniformBlock";
    }
    return "Unknown Descriptor Type";
}
}
