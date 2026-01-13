//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/reference.h"
#include "portal/engine/renderer/image/image.h"
#include "portal/engine/renderer/vulkan/allocated_image.h"
#include "portal/engine/renderer/vulkan/base/allocated.h"
#include "portal/engine/renderer/vulkan/image/vulkan_sampler.h"

namespace portal::renderer::vulkan
{
class VulkanContext;

class VulkanImageView;

/**
 * @struct VulkanImageInfo
 * @brief Vulkan image resources (allocation, view, sampler)
 */
struct VulkanImageInfo
{
    ImageAllocation image = nullptr;
    Reference<VulkanImageView> view = nullptr;
    Reference<VulkanSampler> sampler = nullptr;
};

/**
 * @class VulkanImage
 * @brief Vulkan image with VMA allocation and per-mip/layer views
 *
 * Supports two creation modes:
 * - Wrapping existing vk::Image (e.g., swapchain images)
 * - Allocating new image with VMA
 *
 * Provides per-mip and per-layer image views for compute/fragment shader access.
 */
class VulkanImage final : public Image
{
public:
    /**
     * @brief Wraps existing Vulkan image (e.g., swapchain image)
     * @param image Vulkan image handle
     * @param properties Image properties
     * @param context Vulkan context
     */
    VulkanImage(vk::Image image, const image::Properties& properties, const VulkanContext& context);

    /**
     * @brief Allocates new Vulkan image with VMA
     * @param properties Image properties
     * @param context Vulkan context
     */
    VulkanImage(const image::Properties& properties, const VulkanContext& context);

    ~VulkanImage() override;

    /** @brief Reallocates image with current properties */
    void reallocate() override;

    /** @brief Resizes image (recreates GPU allocation) */
    void resize(size_t width, size_t height) override;

    /** @brief Releases GPU resources */
    void release() override;

    /** @brief Checks if image is valid */
    [[nodiscard]] bool is_image_valid() const;

    /** @brief Gets image width */
    [[nodiscard]] size_t get_width() const override;

    /** @brief Gets image height */
    [[nodiscard]] size_t get_height() const override;

    /** @brief Gets image size as 2D vector */
    [[nodiscard]] glm::uvec2 get_size() const override;

    /** @brief Gets Vulkan format */
    [[nodiscard]] vk::Format get_format() const;

    /** @brief Checks if image has mipmaps */
    [[nodiscard]] bool has_mip() const override;

    /** @brief Gets aspect ratio (width/height) */
    [[nodiscard]] float get_aspect_ratio() const override;

    /** @brief Gets image view */
    [[nodiscard]] Reference<ImageView> get_view() const override;

    /**
     * @brief Finds closest mip level for dimensions
     * @param width Target width
     * @param height Target height
     * @return Closest mip level index
     */
    int get_closest_mip_level(size_t width, size_t height) const;

    /**
     * @brief Gets dimensions of mip level
     * @param mip_level Mip level index
     * @return Pair of (width, height)
     */
    std::pair<size_t, size_t> get_mip_level_dimensions(size_t mip_level) const;

    /** @brief Gets image properties (mutable) */
    [[nodiscard]] image::Properties& get_prop();

    /** @brief Gets image properties */
    [[nodiscard]] const image::Properties& get_prop() const override;

    /** @brief Creates per-layer views for array/cube textures */
    void create_per_layer_image_view() override;

    /**
     * @brief Gets or creates image view for specific mip level
     * @param mip_level Mip level index
     * @return Vulkan image view
     */
    Reference<VulkanImageView> get_mip_image_view(size_t mip_level);

    /**
     * @brief Gets image view for specific layer
     * @param layer Layer index
     * @return Vulkan image view
     */
    Reference<VulkanImageView> get_layer_image_view(size_t layer);

    /** @brief Gets image info (allocation, view, sampler) */
    VulkanImageInfo& get_image_info();

    /** @brief Gets descriptor image info for binding */
    const vk::DescriptorImageInfo& get_descriptor_image_info() const;

    /** @brief Gets Vulkan image allocation */
    [[nodiscard]] const ImageAllocation& get_image() const;

    /** @brief Gets Vulkan sampler */
    [[nodiscard]] const Reference<VulkanSampler>& get_sampler() const;

    /** @brief Gets CPU buffer (const) */
    [[nodiscard]] Buffer get_buffer() const override;

    /** @brief Gets CPU buffer (mutable) */
    [[nodiscard]] Buffer& get_buffer() override;

    /**
     * @brief Uploads data to GPU
     * @param buffer CPU buffer to upload
     */
    void set_data(Buffer buffer) override;

    /**
     * @brief Downloads GPU data to CPU buffer
     * @return CPU buffer with image data
     */
    Buffer copy_to_host_buffer() override;

    /** @brief Updates descriptor image info */
    void update_descriptor();

private:
    const VulkanContext& context;
    const VulkanDevice& device;
    image::Properties properties;

    Buffer image_data;

    VulkanImageInfo image_info;

    std::vector<Reference<VulkanImageView>> per_layer_image_views;
    std::unordered_map<size_t, Reference<VulkanImageView>> per_mip_image_views;
    vk::DescriptorImageInfo descriptor_image_info;
};

class VulkanImageView final : public ImageView
{
public:
    explicit VulkanImageView(vk::ImageView image_view, const ImageViewProperties& image_view_properties, const VulkanContext& context);
    explicit VulkanImageView(const ImageViewProperties& image_view_properties, const VulkanContext& context);

    ~VulkanImageView() override;

    [[nodiscard]] vk::ImageView get_vk_image_view() const;

private:
    const VulkanContext& context;
    vk::ImageView image_view;
    const bool owner;
};
} // vulkan
