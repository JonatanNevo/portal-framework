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

void renderer::ShaderUniform::serialize(Serializer& s) const
{
    s.add_value(name);
    s.add_value(property.type);
    s.add_value(property.container_type);
    s.add_value(property.elements_number);
    s.add_value(offset);
    s.add_value(size);
}

renderer::ShaderUniform renderer::ShaderUniform::deserialize(Deserializer& d)
{
    ShaderUniform uniform;

    d.get_value(uniform.name);
    d.get_value(uniform.property.type);
    d.get_value(uniform.property.container_type);
    d.get_value(uniform.property.elements_number);
    d.get_value(uniform.offset);
    d.get_value(uniform.size);

    return uniform;
}

void renderer::ShaderBuffer::serialize(Serializer& s) const
{
    s.add_value(name.string);
    s.add_value(size);
    s.add_value(uniforms);
}

renderer::ShaderBuffer renderer::ShaderBuffer::deserialize(Deserializer& d)
{
    ShaderBuffer buffer;

    d.get_value(buffer.name);
    d.get_value(buffer.size);
    d.get_value(buffer.uniforms);

    return buffer;
}

void renderer::ShaderResourceDeclaration::serialize(Serializer& s) const
{
    s.add_value(name);
    s.add_value(set);
    s.add_value(binding_index);
    s.add_value(count);
}

renderer::ShaderResourceDeclaration renderer::ShaderResourceDeclaration::deserialize(Deserializer& d)
{
    ShaderResourceDeclaration declaration;

    d.get_value(declaration.name);
    d.get_value(declaration.set);
    d.get_value(declaration.binding_index);
    d.get_value(declaration.count);

    return declaration;
}
}
