//
// Created by Jonatan Nevo on 03/03/2025.
//

#include "image_view.h"

#include "portal/application/vulkan/device.h"
#include "portal/application/vulkan/image.h"

#include <vulkan/vulkan_format_traits.hpp>

namespace portal::vulkan
{
ImageView::ImageView(
    Image& image,
    const vk::ImageViewType type,
    const vk::Format format,
    const uint32_t base_mip_level,
    const uint32_t base_array_layer,
    const uint32_t n_mip_levels,
    const uint32_t n_array_layers
): VulkanResource(nullptr, &image.get_device()), image(&image), format(format)
{
    if (format == vk::Format::eUndefined)
        this->format = image.get_format();

    subresource_range = vk::ImageSubresourceRange(
        (std::string(vk::componentName(format, 0)) == "D") ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor,
        base_mip_level,
        n_mip_levels == 0 ? image.get_subresource().mipLevel : n_mip_levels,
        base_array_layer,
        n_array_layers == 0 ? image.get_subresource().arrayLayer : n_array_layers
    );

    const vk::ImageViewCreateInfo view_info({}, image.get_handle(), type, format, {}, subresource_range);
    set_handle(get_device().get_handle().createImageView(view_info));

    // Register this image view to its image
    // in order to be notified when it gets moved
    image.get_views().emplace(this);
}

ImageView::ImageView(ImageView&& other) noexcept:
    VulkanResource{std::move(other)},
    image{other.image},
    format{other.format},
    subresource_range{other.subresource_range}
{
    // Remove old view from image set and add this new one
    auto& views = image->get_views();
    views.erase(&other);
    views.emplace(this);
    other.set_handle(nullptr);
}

ImageView::~ImageView()
{
    if (get_handle())
        get_device().get_handle().destroyImageView(get_handle());
}

vk::Format ImageView::get_format() const
{
    return format;
}

Image& ImageView::get_image() const
{
    PORTAL_CORE_ASSERT(image, "Image view is referencing an invalid image");
    return *image;
}

void ImageView::set_image(Image& image)
{
    this->image = &image;
}

vk::ImageSubresourceLayers ImageView::get_subresource_layers() const
{
    return vk::ImageSubresourceLayers(
        subresource_range.aspectMask,
        subresource_range.baseMipLevel,
        subresource_range.baseArrayLayer,
        subresource_range.layerCount
    );
}

vk::ImageSubresourceRange ImageView::get_subresource_range() const
{
    return subresource_range;
}
}
