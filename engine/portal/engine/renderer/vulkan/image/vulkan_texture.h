//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/reference.h"
#include "portal/engine/renderer/image/texture.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"

namespace portal::renderer
{
class Sampler;
}

namespace portal::renderer::vulkan
{
class VulkanContext;
class VulkanImage;


class VulkanTexture final : public Texture
{
public:
    explicit VulkanTexture(const StringId& id, const TextureProperties& properties, const Buffer& data, const VulkanContext& context);

    void resize(const glm::uvec3& size) override;
    void resize(size_t width, size_t height, size_t depth) override;

    void update_image();

    void set_sampler(const Reference<Sampler>& sampler);

    [[nodiscard]] ImageFormat get_format() const override;

    [[nodiscard]] size_t get_width() const override;
    [[nodiscard]] size_t get_height() const override;
    [[nodiscard]] size_t get_depth() const override;

    [[nodiscard]] glm::uvec3 get_size() const override;

    [[nodiscard]] uint32_t get_mip_level_count() const override;
    [[nodiscard]] glm::uvec3 get_mip_size(uint32_t mip) const override;

    [[nodiscard]] Reference<Image> get_image() const override;

    Buffer get_writeable_buffer() override;

    [[nodiscard]] TextureType get_type() const override;
    [[nodiscard]] const vk::DescriptorImageInfo& get_descriptor_image_info() const;

    bool loaded() const override;

private:
    void recreate();

    void set_data(const Buffer& data);
    void generate_mipmaps() const;

    uint32_t get_array_layer_count() const;

private:
    TextureProperties properties;
    Buffer image_data;

    const VulkanContext& context;
    const VulkanDevice& device;

    Reference<VulkanImage> image;
};
} // portal
