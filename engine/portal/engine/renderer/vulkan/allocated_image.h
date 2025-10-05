//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/reference.h>

#include "portal/engine/renderer/vulkan/base/allocated.h"
#include "portal/engine/renderer/vulkan/base/builder_base.h"

namespace portal::renderer::vulkan
{
class AllocatedImage;
class VulkanDevice;

struct ImageBuilder final : BuilderBase<ImageBuilder, vk::ImageCreateInfo>
{
public:
    explicit ImageBuilder(const vk::Extent3D& extent);
    explicit ImageBuilder(const vk::Extent2D& extent);
    explicit ImageBuilder(size_t width, size_t height = 1, size_t depth = 1);

    ImageBuilder& with_format(vk::Format format);
    ImageBuilder& with_image_type(vk::ImageType type);
    ImageBuilder& with_array_layers(size_t layers);
    ImageBuilder& with_mips_levels(size_t levels);
    ImageBuilder& with_sample_count(vk::SampleCountFlagBits sample_count);
    ImageBuilder& with_tiling(vk::ImageTiling tiling);
    ImageBuilder& with_usage(vk::ImageUsageFlags usage);
    ImageBuilder& with_flags(vk::ImageCreateFlags flags);

    AllocatedImage build(Ref<VulkanDevice> device) const;
};

class AllocatedImage final : public allocation::Allocated<vk::Image>
{
public:
    AllocatedImage();
    AllocatedImage(nullptr_t);

    AllocatedImage(AllocatedImage&& other) noexcept;
    AllocatedImage& operator=(AllocatedImage&& other) noexcept;
    AllocatedImage& operator=(nullptr_t) noexcept override;

    AllocatedImage(const AllocatedImage&) = delete;
    AllocatedImage& operator=(const AllocatedImage&) = delete;

    ~AllocatedImage() override;
protected:
    friend struct ImageBuilder;
    AllocatedImage(Ref<VulkanDevice> device, const ImageBuilder& builder);
};
} // portal