//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <memory>
#include <vector>

#include "portal/engine/reference.h"
#include "portal/engine/renderer/renderer_resource.h"
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
    std::vector<Reference<RendererResource>> input;

    DescriptorInput() = default;
    DescriptorInput(const Reference<UniformBuffer>& buffer);
    DescriptorInput(const Reference<UniformBufferSet>& buffer);

    DescriptorInput(const Reference<StorageBuffer>& buffer);
    DescriptorInput(const Reference<StorageBufferSet>& buffer);

    DescriptorInput(const Reference<Texture>& texture);

    DescriptorInput(const Reference<Image>& texture);

    void set(const Reference<UniformBuffer>& buffer, size_t index = 0);
    void set(const Reference<UniformBufferSet>& buffer, size_t index = 0);

    void set(const Reference<StorageBuffer>& buffer, size_t index = 0);
    void set(const Reference<StorageBufferSet>& buffer, size_t index = 0);

    void set(const Reference<Texture>& texture, size_t index = 0);

    void set(const Reference<Image>& image, size_t index = 0);
    void set(const Reference<ImageView>& image, size_t index = 0);
};
}
