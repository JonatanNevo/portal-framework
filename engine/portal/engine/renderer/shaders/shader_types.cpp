//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "shader_types.h"

#include "portal/serialization/serialize.h"

namespace portal
{

std::string utils::to_string(const renderer::DescriptorType type)
{
    switch (type)
    {
    case renderer::DescriptorType::Unknown:
        return "Unknown Descriptor Type";
    case renderer::DescriptorType::Sampler:
        return "Sampler";
    case renderer::DescriptorType::CombinedImageSampler:
        return "CombinedImageSampler";
    case renderer::DescriptorType::SampledImage:
        return "SampledImage";
    case renderer::DescriptorType::StorageImage:
        return "StorageImage";
    case renderer::DescriptorType::UniformTexelBuffer:
        return "UniformTexelBuffer";
    case renderer::DescriptorType::StorageTexelBuffer:
        return "StorageTexelBuffer";
    case renderer::DescriptorType::UniformBuffer:
        return "UniformBuffer";
    case renderer::DescriptorType::StorageBuffer:
        return "StorageBuffer";
    case renderer::DescriptorType::UniformBufferDynamic:
        return "UniformBufferDynamic";
    case renderer::DescriptorType::StorageBufferDynamic:
        return "StorageBufferDynamic";
    case renderer::DescriptorType::InputAttachment:
        return "InputAttachment";
    case renderer::DescriptorType::AccelerationStructure:
        return "AccelerationStructure";
    case renderer::DescriptorType::InlineUniformBlock:
        return "InlineUniformBlock";
    }
    return "Unknown Descriptor Type";
}
}
