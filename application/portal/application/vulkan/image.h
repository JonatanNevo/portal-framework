//
// Created by Jonatan Nevo on 03/03/2025.
//

#pragma once
#include <unordered_set>

#include "base/allocated.h"
#include "portal/application/vulkan/base/builder_base.h"

namespace portal::vulkan
{
class ImageView;
class Image;

struct ImageBuilder : public allocated::BuilderBase<ImageBuilder, vk::ImageCreateInfo>
{
public:
    ImageBuilder(const vk::Extent3D& extent);
    ImageBuilder(const vk::Extent2D& extent);
    ImageBuilder(uint32_t width, uint32_t height = 1, uint32_t depth = 1);
    ImageBuilder& with_format(vk::Format format);
    ImageBuilder& with_image_type(vk::ImageType type);
    ImageBuilder& with_array_layers(uint32_t layers);
    ImageBuilder& with_mips_levels(uint32_t levels);
    ImageBuilder& with_sample_count(vk::SampleCountFlagBits sample_count);
    ImageBuilder& with_tiling(vk::ImageTiling tiling);
    ImageBuilder& with_usage(vk::ImageUsageFlags usage);
    ImageBuilder& with_flags(vk::ImageCreateFlags flags);

    Image build(Device& device) const;
    std::unique_ptr<Image> build_unique(Device& device) const;
};

class Image final : public allocated::Allocated<vk::Image>
{
public:
    Image(
        Device& device,
        vk::Image handle,
        const vk::Extent3D& extent,
        vk::Format format,
        vk::ImageUsageFlags image_usage,
        vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1
    );

    Image(const Image&) = delete;
    Image(Image&& other) noexcept;
    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) = delete;

    ~Image() override;

    /**
     * @brief Maps vulkan memory to a host visible address
     * @return Pointer to host visible memory
     */
    uint8_t* map() override;

    vk::ImageType get_type() const;
    const vk::Extent3D& get_extent() const;
    vk::Format get_format() const;
    vk::SampleCountFlagBits get_sample_count() const;
    vk::ImageUsageFlags get_usage() const;
    vk::ImageTiling get_tiling() const;
    vk::ImageSubresource get_subresource() const;
    uint32_t get_array_layer_count() const;
    std::unordered_set<ImageView*>& get_views();

protected:
    friend struct ImageBuilder;
    Image(Device& device, ImageBuilder const& builder);

private:
    vk::ImageCreateInfo create_info;
    vk::ImageSubresource subresource;
    std::unordered_set<ImageView*> views; /// HPPImage views referring to this image
};
} // portal
