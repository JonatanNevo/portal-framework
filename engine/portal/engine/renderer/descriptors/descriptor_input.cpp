//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "descriptor_input.h"
#include "portal/engine/renderer/descriptors/uniform_buffer.h"
#include "portal/engine/renderer/descriptors/storage_buffer.h"
#include "portal/engine/renderer/image/texture.h"
#include "portal/engine/renderer/image/image.h"


namespace portal::renderer
{

DescriptorInput::DescriptorInput(Ref<UniformBuffer> buffer): type(DescriptorResourceType::UniformBuffer), input({1, buffer})
{
}

DescriptorInput::DescriptorInput(Ref<UniformBufferSet> buffer): type(DescriptorResourceType::UniformBufferSet), input({1, buffer})
{
}

DescriptorInput::DescriptorInput(Ref<StorageBuffer> buffer): type(DescriptorResourceType::StorageBuffer), input({1, buffer})
{
}

DescriptorInput::DescriptorInput(Ref<StorageBufferSet> buffer): type(DescriptorResourceType::StorageBufferSet), input({1, buffer})
{
}

DescriptorInput::DescriptorInput(Ref<Texture> texture): type(DescriptorResourceType::Texture), input({1, texture})
{
}

DescriptorInput::DescriptorInput(Ref<Image> texture): type(DescriptorResourceType::Image), input({1, texture})
{
}

void DescriptorInput::set(const Ref<UniformBuffer>& buffer, const size_t index)
{
    type = DescriptorResourceType::UniformBuffer;
    input[index] = buffer;
}

void DescriptorInput::set(const Ref<UniformBufferSet>& buffer, const size_t index)
{
    type = DescriptorResourceType::UniformBufferSet;
    input[index] = buffer;
}

void DescriptorInput::set(const Ref<StorageBuffer>& buffer, const size_t index)
{
    type = DescriptorResourceType::StorageBuffer;
    input[index] = buffer;
}

void DescriptorInput::set(const Ref<StorageBufferSet>& buffer, const size_t index)
{
    type = DescriptorResourceType::StorageBufferSet;
    input[index] = buffer;
}

void DescriptorInput::set(const Ref<Texture>& texture, const size_t index)
{
    type = DescriptorResourceType::Texture;
    input[index] = texture;
}

void DescriptorInput::set(const Ref<Image>& image, const size_t index)
{
    type = DescriptorResourceType::Image;
    input[index] = image;
}

void DescriptorInput::set(const Ref<ImageView>& image, const size_t index)
{
    type = DescriptorResourceType::Image;
    input[index] = image;
}
}
