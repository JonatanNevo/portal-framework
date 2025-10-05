//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/reference.h"
#include "portal/engine/renderer/descriptors/descriptor_types.h"


namespace portal::renderer
{

class UniformBufferSet;
class UniformBuffer;
class Texture;
class StorageBufferSet;
class StorageBuffer;
class Image;
class ImageView;

struct DescriptorInput
{
    DescriptorResourceType type = DescriptorResourceType::Unknown;
    std::vector<Ref<RefCounted>> input;

    DescriptorInput() = default;
    DescriptorInput(Ref<UniformBuffer> buffer);
    DescriptorInput(Ref<UniformBufferSet> buffer);

    DescriptorInput(Ref<StorageBuffer> buffer);
    DescriptorInput(Ref<StorageBufferSet> buffer);

    DescriptorInput(Ref<Texture> texture);

    DescriptorInput(Ref<Image> texture);

    void set(const Ref<UniformBuffer>& buffer, size_t index = 0);
    void set(const Ref<UniformBufferSet>& buffer, size_t index = 0);

    void set(const Ref<StorageBuffer>& buffer, size_t index = 0);
    void set(const Ref<StorageBufferSet>& buffer, size_t index = 0);

    void set(const Ref<Texture>& texture, size_t index = 0);

    void set(const Ref<Image>& image, size_t index = 0);
    void set(const Ref<ImageView>& image, size_t index = 0);
};


}
