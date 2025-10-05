//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/renderer/image/texture.h"
#include "portal/engine/renderer/vulkan/vulkan_device.h"

namespace portal::renderer {
class Sampler;
}

namespace portal::renderer::vulkan
{
class VulkanContext;
class VulkanImage;


class VulkanTexture final : public Texture
{
public:
    explicit VulkanTexture(const StringId& id);

    void initialize(const TextureSpecification& new_spec, const Buffer& data, Ref<VulkanContext> new_context);
    void copy_from(Ref<Resource>) override;

    void resize(const glm::uvec3& size) override;
    void resize(size_t width, size_t height, size_t depth) override;

    void update_image();

    void set_sampler(const Ref<Sampler>& sampler);

    ImageFormat get_format() const override;

    size_t get_width() const override;
    size_t get_height() const override;
    size_t get_depth() const override;

    glm::uvec3 get_size() const override;

    uint32_t get_mip_level_count() const override;
    glm::uvec3 get_mip_size(uint32_t mip) const override;

    Ref<Image> get_image() const override;

    Buffer get_writeable_buffer() override;

    TextureType get_type() const override;
    [[nodiscard]] const vk::DescriptorImageInfo& get_descriptor_image_info() const;

    bool loaded() const override;

private:
    void recreate();

    void set_data(const Buffer& data);
    void generate_mipmaps();

    uint32_t get_array_layer_count() const;

private:
    TextureSpecification spec;
    Buffer image_data;

    Ref<VulkanImage> image;
    Ref<VulkanContext> context;
    Ref<VulkanDevice> device;
};

} // portal