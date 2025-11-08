//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "allocated_image.h"

#include "portal/engine/renderer/vulkan/vulkan_device.h"

namespace portal::renderer::vulkan
{

inline vk::ImageType find_image_type(const vk::Extent3D& extent)
{
    const uint32_t dim_num = !!extent.width + !!extent.height + (1 < extent.depth);
    switch (dim_num)
    {
    case 1:
        return vk::ImageType::e1D;
    case 2:
        return vk::ImageType::e2D;
    case 3:
        return vk::ImageType::e3D;
    default:
        throw std::runtime_error("No image type found.");
    }
}

ImageBuilder::ImageBuilder(const vk::Extent3D& extent) : BuilderBase(
    vk::ImageCreateInfo{
        .imageType = find_image_type(extent),
        .format = vk::Format::eR8G8B8A8Unorm,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1
    }
    ) {}

ImageBuilder::ImageBuilder(const vk::Extent2D& extent) : ImageBuilder(vk::Extent3D{extent.width, extent.height, 1}) {}

ImageBuilder::ImageBuilder(const size_t width, const size_t height, const size_t depth) : ImageBuilder(
    vk::Extent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), static_cast<uint32_t>(depth)}
    ) {}

ImageBuilder& ImageBuilder::with_format(const vk::Format format)
{
    create_info.format = format;
    return *this;
}

ImageBuilder& ImageBuilder::with_image_type(const vk::ImageType type)
{
    create_info.imageType = type;
    return *this;
}

ImageBuilder& ImageBuilder::with_array_layers(const size_t layers)
{
    create_info.arrayLayers = static_cast<uint32_t>(layers);
    return *this;
}

ImageBuilder& ImageBuilder::with_mips_levels(const size_t levels)
{
    create_info.mipLevels = static_cast<uint32_t>(levels);
    return *this;
}

ImageBuilder& ImageBuilder::with_sample_count(const vk::SampleCountFlagBits sample_count)
{
    create_info.samples = sample_count;
    return *this;
}

ImageBuilder& ImageBuilder::with_tiling(const vk::ImageTiling tiling)
{
    create_info.tiling = tiling;
    return *this;
}

ImageBuilder& ImageBuilder::with_usage(const vk::ImageUsageFlags usage)
{
    create_info.usage = usage;
    return *this;
}

ImageBuilder& ImageBuilder::with_flags(const vk::ImageCreateFlags flags)
{
    create_info.flags = flags;
    return *this;
}

AllocatedImage ImageBuilder::build(const VulkanDevice& device) const
{
    return {device, *this};
}

AllocatedImage::AllocatedImage() : Allocated({}, nullptr, nullptr) {}

AllocatedImage::AllocatedImage(nullptr_t) : AllocatedImage() {}

AllocatedImage::AllocatedImage(AllocatedImage&& other) noexcept : Allocated(std::move(other))
{}

AllocatedImage& AllocatedImage::operator=(AllocatedImage&& other) noexcept
{
    if (this != &other)
    {
        destroy_image(get_handle());
        Allocated::operator=(std::move(other));
    }
    return *this;
}

AllocatedImage& AllocatedImage::operator=(const nullptr_t nullptr_) noexcept
{
    destroy_image(get_handle());
    Allocated::operator=(nullptr_);
    return *this;
}

AllocatedImage::~AllocatedImage()
{
    destroy_image(get_handle());
}

AllocatedImage::AllocatedImage(const VulkanDevice& device, const ImageBuilder& builder) :
    Allocated(builder.get_allocation_create_info(), nullptr, &device)
{
    set_handle(create_image(builder.get_create_info()));
    set_debug_name(builder.get_debug_name().c_str());
}
} // portal
