//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/buffer.h"
#include "portal/engine/renderer/allocated_image.h"
#include "portal/engine/resources/resources/resource.h"

namespace portal
{
namespace resources {
    class GltfLoader;
    class TextureLoader;
}

class Texture final : public Resource
{
public:
    const static StringId MISSING_TEXTURE_ID;
    const static StringId WHITE_TEXTURE_ID;
    const static StringId BLACK_TEXTURE_ID;

    Texture() = default;
    explicit Texture(const StringId& id) : Resource(id) {}
    void copy_from(Ref<Resource> other) override;

    [[nodiscard]] const vulkan::AllocatedImage& get_image() const { return *image; }
    [[nodiscard]] const Buffer& get_data() const { return data; }

    vulkan::AllocatedImage& get();
    const vulkan::AllocatedImage& get() const;
    vk::raii::Sampler& get_sampler();
    const vk::raii::Sampler& get_sampler() const;

private:
    friend class resources::TextureLoader;
    friend class resources::GltfLoader;

    std::shared_ptr<vulkan::AllocatedImage> image = nullptr;
    std::shared_ptr<vk::raii::Sampler> sampler = nullptr;

    Buffer data;
};

} // portal
