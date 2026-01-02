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

/**
 * @class VulkanTexture
 * @brief Vulkan texture with automatic mipmap generation and sampler management
 *
 * Wraps VulkanImage with texture-specific functionality: automatic mipmap generation,
 * CPU buffer management, sampler configuration, and texture type support (2D/cube).
 */
class VulkanTexture final : public Texture
{
public:
    /**
     * @brief Constructs Vulkan texture
     * @param id Texture ID
     * @param properties Texture properties
     * @param data CPU texture data
     * @param context Vulkan context
     */
    explicit VulkanTexture(const StringId& id, const TextureProperties& properties, const Buffer& data, const VulkanContext& context);

    /**
     * @brief Resizes texture
     * @param size New size
     */
    void resize(const glm::uvec3& size) override;

    /**
     * @brief Resizes texture
     * @param width New width
     * @param height New height
     * @param depth New depth
     */
    void resize(size_t width, size_t height, size_t depth) override;

    /** @brief Updates GPU image from CPU buffer */
    void update_image();

    /**
     * @brief Sets texture sampler
     * @param sampler Sampler to use
     */
    void set_sampler(const Reference<Sampler>& sampler);

    /** @brief Gets texture format */
    [[nodiscard]] ImageFormat get_format() const override;

    /** @brief Gets texture width */
    [[nodiscard]] size_t get_width() const override;

    /** @brief Gets texture height */
    [[nodiscard]] size_t get_height() const override;

    /** @brief Gets texture depth */
    [[nodiscard]] size_t get_depth() const override;

    /** @brief Gets texture size as 3D vector */
    [[nodiscard]] glm::uvec3 get_size() const override;

    /** @brief Gets mipmap level count */
    [[nodiscard]] uint32_t get_mip_level_count() const override;

    /**
     * @brief Gets dimensions of mip level
     * @param mip Mip level index
     * @return Mip size (width/height/depth)
     */
    [[nodiscard]] glm::uvec3 get_mip_size(uint32_t mip) const override;

    /** @brief Gets underlying Vulkan image */
    [[nodiscard]] Reference<Image> get_image() const override;

    /** @brief Gets CPU buffer (const) */
    [[nodiscard]] Buffer get_buffer() const override;

    /** @brief Gets CPU buffer (mutable) */
    Buffer get_writeable_buffer() override;

    /** @brief Gets texture type (2D/cube) */
    [[nodiscard]] TextureType get_type() const override;

    /** @brief Gets descriptor image info for binding */
    [[nodiscard]] const vk::DescriptorImageInfo& get_descriptor_image_info() const;

    /** @brief Checks if texture data is loaded */
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
