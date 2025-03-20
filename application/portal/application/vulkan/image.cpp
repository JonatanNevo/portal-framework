//
// Created by Jonatan Nevo on 03/03/2025.
//

#include "image.h"

#include "portal/application/vulkan/image_view.h"
#include "portal/application/vulkan/device.h"

namespace portal::vulkan
{
inline vk::ImageType find_image_type(vk::Extent3D const& extent)
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
        return vk::ImageType();
    }
}

ImageBuilder::ImageBuilder(const vk::Extent3D& extent): BuilderBase(
    vk::ImageCreateInfo{{}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm, extent, 1, 1}
) {}

ImageBuilder::ImageBuilder(const vk::Extent2D& extent): ImageBuilder(vk::Extent3D(extent, 1)) {}

ImageBuilder::ImageBuilder(const uint32_t width, const uint32_t height, const uint32_t depth): ImageBuilder(vk::Extent3D(width, height, depth)) {}

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

ImageBuilder& ImageBuilder::with_array_layers(const uint32_t layers)
{
    create_info.arrayLayers = layers;
    return *this;
}

ImageBuilder& ImageBuilder::with_mips_levels(const uint32_t levels)
{
    create_info.mipLevels = levels;
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

Image ImageBuilder::build(Device& device) const
{
    return {device, *this};
}

std::unique_ptr<Image> ImageBuilder::build_unique(Device& device) const
{
    return std::unique_ptr<Image>(new Image(device, *this));
}

////////////////////////////////////////////////////////////////////////////////

Image::Image(
    Device& device,
    vk::Image handle,
    const vk::Extent3D& extent,
    vk::Format format,
    vk::ImageUsageFlags image_usage,
    vk::SampleCountFlagBits sample_count
): Allocated(handle, &device)
{
    create_info.samples = sample_count;
    create_info.format = format;
    create_info.extent = extent;
    create_info.imageType = find_image_type(extent);
    create_info.arrayLayers = 1;
    create_info.mipLevels = 1;
    subresource.mipLevel = 1;
    subresource.arrayLayer = 1;
}


Image::Image(Image&& other) noexcept:
    allocated::Allocated<vk::Image>(std::move(other)),
    create_info(std::exchange(other.create_info, {})),
    subresource(std::exchange(other.subresource, {})),
    views(std::exchange(other.views, {}))
{
    // Update image views references to this image to avoid dangling pointers
    for (auto& view : views)
    {
        view->set_image(*this);
    }
}

Image::~Image()
{
    destroy_image(get_handle());
}

uint8_t* Image::map()
{
    if (create_info.tiling != vk::ImageTiling::eLinear)
        LOG_CORE_WARN_TAG("Vulkan", "Mapping image memory that is not linear");
    return Allocated::map();
}

vk::ImageType Image::get_type() const
{
    return create_info.imageType;
}

const vk::Extent3D& Image::get_extent() const
{
    return create_info.extent;
}

vk::Format Image::get_format() const
{
    return create_info.format;
}

vk::SampleCountFlagBits Image::get_sample_count() const
{
    return create_info.samples;
}

vk::ImageUsageFlags Image::get_usage() const
{
    return create_info.usage;
}

vk::ImageTiling Image::get_tiling() const
{
    return create_info.tiling;
}

vk::ImageSubresource Image::get_subresource() const
{
    return subresource;
}

uint32_t Image::get_array_layer_count() const
{
    return create_info.arrayLayers;
}

std::unordered_set<ImageView*>& Image::get_views()
{
    return views;
}

Image::Image(Device& device, ImageBuilder const& builder)
    : Allocated(builder.get_allocation_create_info(), nullptr, &device),
      create_info(builder.get_create_info())
{
    set_handle(create_image(create_info));
    subresource.arrayLayer = create_info.arrayLayers;
    subresource.mipLevel = create_info.mipLevels;
    if (!builder.get_debug_name().empty())
        set_debug_name(builder.get_debug_name());
}
} // portal
