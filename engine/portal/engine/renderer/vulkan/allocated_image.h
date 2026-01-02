//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once


#include "portal/engine/renderer/vulkan/base/allocated.h"
#include "portal/engine/renderer/vulkan/base/builder_base.h"

namespace portal::renderer::vulkan
{
class ImageAllocation;
class VulkanDevice;

/**
 * @struct ImageBuilder
 * @brief Builder for creating VMA-allocated Vulkan images
 *
 * Inherits common VMA options from BuilderBase. Extent is required at construction.
 * Supports 1D, 2D, and 3D images with configurable format, mip levels, array layers, etc.
 *
 * Usage:
 * @code
 * auto image = ImageBuilder(vk::Extent2D{1024, 1024})
 *     .with_format(vk::Format::eR8G8B8A8Unorm)
 *     .with_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
 *     .with_mips_levels(mip_count)
 *     .build(device);
 * @endcode
 */
struct ImageBuilder final : BuilderBase<ImageBuilder, vk::ImageCreateInfo>
{
public:
    /**
     * @brief Constructs image builder with 3D extent
     * @param extent Image extent (width, height, depth)
     */
    explicit ImageBuilder(const vk::Extent3D& extent);

    /**
     * @brief Constructs image builder with 2D extent
     * @param extent Image extent (width, height)
     */
    explicit ImageBuilder(const vk::Extent2D& extent);

    /**
     * @brief Constructs image builder with dimensions
     * @param width Image width
     * @param height Image height (default 1)
     * @param depth Image depth (default 1)
     */
    explicit ImageBuilder(size_t width, size_t height = 1, size_t depth = 1);

    /**
     * @brief Sets image format
     * @param format Image format (eR8G8B8A8Unorm, eD32Sfloat, etc.)
     * @return Reference to this builder
     */
    ImageBuilder& with_format(vk::Format format);

    /**
     * @brief Sets image type
     * @param type Image type (e1D, e2D, e3D)
     * @return Reference to this builder
     */
    ImageBuilder& with_image_type(vk::ImageType type);

    /**
     * @brief Sets array layer count
     * @param layers Number of array layers
     * @return Reference to this builder
     */
    ImageBuilder& with_array_layers(size_t layers);

    /**
     * @brief Sets mip level count
     * @param levels Number of mip levels
     * @return Reference to this builder
     */
    ImageBuilder& with_mips_levels(size_t levels);

    /**
     * @brief Sets sample count (for MSAA)
     * @param sample_count Sample count (e1, e2, e4, e8, etc.)
     * @return Reference to this builder
     */
    ImageBuilder& with_sample_count(vk::SampleCountFlagBits sample_count);

    /**
     * @brief Sets image tiling mode
     * @param tiling Tiling mode (eOptimal or eLinear)
     * @return Reference to this builder
     */
    ImageBuilder& with_tiling(vk::ImageTiling tiling);

    /**
     * @brief Sets image usage flags
     * @param usage Usage flags (eSampled, eStorage, eColorAttachment, etc.)
     * @return Reference to this builder
     */
    ImageBuilder& with_usage(vk::ImageUsageFlags usage);

    /**
     * @brief Sets image create flags
     * @param flags Create flags (eCubeCompatible, eMutableFormat, etc.)
     * @return Reference to this builder
     */
    ImageBuilder& with_flags(vk::ImageCreateFlags flags);

    /**
     * @brief Creates ImageAllocation
     * @param device The Vulkan device
     * @return ImageAllocation with VMA-allocated memory
     */
    ImageAllocation build(const VulkanDevice& device) const;
};

/**
 * @class ImageAllocation
 * @brief VMA-allocated Vulkan image with automatic memory management
 *
 * Inherits from Allocated<vk::Image>, providing VMA memory allocation/deallocation.
 * Can also wrap pre-existing images (e.g., swapchain images) without ownership.
 *
 * Destruction automatically calls vmaDestroyImage to free both image and backing memory
 * (if VMA-allocated).
 */
class ImageAllocation final : public allocation::Allocated<vk::Image>
{
public:
    /** @brief Default constructor (creates null image) */
    ImageAllocation();

    /** @brief Null constructor */
    ImageAllocation(std::nullptr_t);

    /**
     * @brief Wraps pre-existing image (no VMA allocation)
     * @param image Pre-existing image handle (e.g., swapchain image)
     */
    ImageAllocation(vk::Image image);

    /** @brief Move constructor */
    ImageAllocation(ImageAllocation&& other) noexcept;

    /** @brief Move assignment */
    ImageAllocation& operator=(ImageAllocation&& other) noexcept;

    /** @brief Null assignment */
    ImageAllocation& operator=(std::nullptr_t) noexcept override;

    ImageAllocation(const ImageAllocation&) = delete;
    ImageAllocation& operator=(const ImageAllocation&) = delete;

    /** @brief Destructor (calls vmaDestroyImage if VMA-allocated) */
    ~ImageAllocation() override;

protected:
    /**
     * @brief Protected constructor (use ImageBuilder::build instead)
     * @param device The Vulkan device
     * @param builder Image builder with configuration
     */
    friend struct ImageBuilder;
    ImageAllocation(const VulkanDevice& device, const ImageBuilder& builder);
};
} // portal
