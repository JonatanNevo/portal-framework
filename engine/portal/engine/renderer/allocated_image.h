//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <unordered_set>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

#include "base/builder_base.h"
#include "portal/engine/renderer/base/allocated.h"

namespace portal::vulkan
{

class AllocatedImage;

struct ImageBuilder final : public BuilderBase<ImageBuilder, vk::ImageCreateInfo>
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

    AllocatedImage build(vk::raii::Device& device) const;
    std::shared_ptr<AllocatedImage> build_shared(vk::raii::Device& device) const;
};

class AllocatedImage final : public allocation::Allocated<vk::Image>
{
public:
    AllocatedImage();
    AllocatedImage(nullptr_t): AllocatedImage() {}

    AllocatedImage(AllocatedImage&& other) noexcept;
    AllocatedImage& operator=(AllocatedImage&& other) noexcept;
    AllocatedImage& operator=(nullptr_t) noexcept override;

    AllocatedImage(const AllocatedImage&) = delete;
    AllocatedImage& operator=(const AllocatedImage&) = delete;

    ~AllocatedImage() override;

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
    uint32_t get_mip_levels() const;
    uint32_t get_array_layer_count() const;
    // std::unordered_set<vk::raii::ImageView>& get_views();

    const vk::raii::ImageView& get_view() const;

protected:
    friend struct ImageBuilder;
    AllocatedImage(vk::raii::Device& device, const ImageBuilder& builder);

private:
    vk::ImageCreateInfo create_info;
    vk::ImageSubresource subresource;
    vk::raii::ImageView image_view = nullptr;
    // std::unordered_set<vk::raii::ImageView> views;
};

} // vulkan
