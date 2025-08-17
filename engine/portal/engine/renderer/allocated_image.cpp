//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "allocated_image.h"
#include <vulkan/vulkan_format_traits.hpp>

namespace portal::vulkan
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

ImageBuilder::ImageBuilder(const vk::Extent3D& extent): BuilderBase(
    vk::ImageCreateInfo{
        .imageType = find_image_type(extent),
        .format = vk::Format::eR8G8B8A8Unorm,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1
    }
    ) {}

ImageBuilder::ImageBuilder(const vk::Extent2D& extent): ImageBuilder(vk::Extent3D{extent.width, extent.height, 1}) {}

ImageBuilder::ImageBuilder(uint32_t width, uint32_t height, uint32_t depth): ImageBuilder(vk::Extent3D{width, height, depth}) {}

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

AllocatedImage ImageBuilder::build(vk::raii::Device& device) const
{
    return {device, *this};
}

std::shared_ptr<AllocatedImage> ImageBuilder::build_shared(vk::raii::Device& device) const
{
    return std::shared_ptr<AllocatedImage>(new AllocatedImage(device, *this));
}

AllocatedImage::AllocatedImage(): Allocated({}, nullptr, nullptr) {}

AllocatedImage::AllocatedImage(AllocatedImage&& other) noexcept
    : Allocated(std::move(other)),
      create_info(std::exchange(other.create_info, {})),
      subresource(std::exchange(other.subresource, {})),
      image_view(std::exchange(other.image_view, nullptr))
{}

AllocatedImage& AllocatedImage::operator=(AllocatedImage&& other) noexcept
{
    if (this != &other)
    {
        destroy_image(get_handle());
        create_info = std::exchange(other.create_info, {});
        subresource = std::exchange(other.subresource, {});
        image_view = std::exchange(other.image_view, nullptr);
        Allocated::operator=(std::move(other));
    }
    return *this;
}

AllocatedImage& AllocatedImage::operator=(nullptr_t) noexcept
{
    destroy_image(get_handle());
    Allocated::operator=(nullptr);
    return *this;
}

AllocatedImage::~AllocatedImage()
{
    destroy_image(get_handle());
}

uint8_t* AllocatedImage::map()
{
    if (create_info.tiling != vk::ImageTiling::eLinear)
        LOG_WARN_TAG("Vulkan", "Mapping image memory that is not linear");
    return Allocated::map();
}

vk::ImageType AllocatedImage::get_type() const
{
    return create_info.imageType;
}

const vk::Extent3D& AllocatedImage::get_extent() const
{
    return create_info.extent;
}

vk::Format AllocatedImage::get_format() const
{
    return create_info.format;
}

vk::SampleCountFlagBits AllocatedImage::get_sample_count() const
{
    return create_info.samples;
}

vk::ImageUsageFlags AllocatedImage::get_usage() const
{
    return create_info.usage;
}

vk::ImageTiling AllocatedImage::get_tiling() const
{
    return create_info.tiling;
}

uint32_t AllocatedImage::get_mip_levels() const
{
    return create_info.mipLevels;
}

uint32_t AllocatedImage::get_array_layer_count() const
{
    return create_info.arrayLayers;
}

const vk::raii::ImageView& AllocatedImage::get_view() const
{
    return image_view;
}

// std::unordered_set<vk::raii::ImageView>& AllocatedImage::get_views()
// {
//     return views;
// }

AllocatedImage::AllocatedImage(vk::raii::Device& device, const ImageBuilder& builder):
    Allocated(builder.get_allocation_create_info(), nullptr, &device),
    create_info(builder.get_create_info())
{
    set_handle(create_image(create_info));
    subresource.arrayLayer = create_info.arrayLayers;
    subresource.mipLevel = create_info.mipLevels;

    vk::ImageSubresourceRange subresource_range{
        (std::string(vk::componentName(get_format(), 0)) == "D") ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor,
        0,
        get_mip_levels(),
        0,
        get_array_layer_count()
    };

    auto view_type = vk::ImageViewType::e2D;

    const vk::ImageViewCreateInfo image_view_create_info{
        .image = get_handle(),
        .viewType = view_type,
        .format = get_format(),
        .subresourceRange = subresource_range
    };
    image_view = device.createImageView(image_view_create_info);

    if (!builder.get_debug_name().empty())
    {
        set_debug_name(builder.get_debug_name());
        device.setDebugUtilsObjectNameEXT(
            vk::DebugUtilsObjectNameInfoEXT{
                .objectType = vk::ObjectType::eImage,
                .objectHandle = get_handle_u64(),
                .pObjectName = get_debug_name().c_str()
            }
            );
    }
}
}
