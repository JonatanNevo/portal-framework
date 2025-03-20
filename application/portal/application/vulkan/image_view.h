//
// Created by Jonatan Nevo on 03/03/2025.
//

#pragma once
#include "base/vulkan_resource.h"

namespace portal::vulkan
{
class Device;
class Image;

class ImageView final : public VulkanResource<vk::ImageView>
{
public:
    ImageView(
        Image& image,
        vk::ImageViewType type,
        vk::Format format = vk::Format::eUndefined,
        uint32_t base_mip_level = 0,
        uint32_t base_array_layer = 0,
        uint32_t n_mip_levels = 0,
        uint32_t n_array_layers = 0
    );
    ImageView(ImageView&& other) noexcept;
    ~ImageView() override;

    ImageView(ImageView& other) = delete;
    ImageView& operator=(ImageView& other) = delete;
    ImageView& operator=(ImageView&& other) = delete;

    vk::Format get_format() const;
    Image& get_image() const;
    void set_image(Image& image);
    vk::ImageSubresourceLayers get_subresource_layers() const;
    vk::ImageSubresourceRange get_subresource_range() const;

private:
    Image* image = nullptr;
    vk::Format format;
    vk::ImageSubresourceRange subresource_range;
};
}
