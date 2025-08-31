//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/buffer.h"
#include "../../renderer/vulkan/vulkan_image.h"
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

    [[nodiscard]] const renderer::vulkan::VulkanImage& get_image() const { return *image; }
    [[nodiscard]] renderer::vulkan::VulkanImage& get_image() { return *image; }

    [[nodiscard]] const Buffer& get_data() const { return data; }

    renderer::vulkan::VulkanImage& get();
    const renderer::vulkan::VulkanImage& get() const;
    vk::raii::Sampler& get_sampler();
    const vk::raii::Sampler& get_sampler() const;

private:
    friend class resources::TextureLoader;
    friend class resources::GltfLoader;

    std::shared_ptr<renderer::vulkan::VulkanImage> image = nullptr;
    std::shared_ptr<vk::raii::Sampler> sampler = nullptr;

    Buffer data;
};

} // portal
