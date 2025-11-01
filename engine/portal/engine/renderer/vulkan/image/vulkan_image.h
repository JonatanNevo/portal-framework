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

struct VulkanImageInfo
{
    AllocatedImage image = nullptr;
    vk::raii::ImageView view = nullptr;
    Reference<VulkanSampler> sampler = nullptr;
};

class VulkanImage final : public Image
{
public:
    VulkanImage(const image::Specification& spec, const VulkanContext& context);
    ~VulkanImage() override;

    void reallocate() override;
    void resize(size_t width, size_t height) override;
    void release() override;

    [[nodiscard]] bool is_image_valid() const;

    [[nodiscard]] size_t get_width() const override;
    [[nodiscard]] size_t get_height() const override;
    [[nodiscard]] glm::uvec2 get_size() const override;
    [[nodiscard]] vk::Format get_format() const;
    [[nodiscard]] bool has_mip() const override;

    [[nodiscard]] float get_aspect_ratio() const override;

    int get_closest_mip_level(size_t width, size_t height) const;
    std::pair<size_t, size_t> get_mip_level_dimensions(size_t mip_level) const;

    [[nodiscard]] image::Specification& get_specs();
    [[nodiscard]] const image::Specification& get_spec() const override;

    void create_per_layer_image_view() override;

    vk::ImageView get_mip_image_view(size_t mip_level);
    vk::ImageView get_layer_image_view(size_t layer);

    VulkanImageInfo& get_image_info();
    const vk::DescriptorImageInfo& get_descriptor_image_info() const;

    [[nodiscard]] Buffer get_buffer() const override;
    [[nodiscard]] Buffer& get_buffer() override;

    void set_data(Buffer buffer) override;
    Buffer copy_to_host_buffer() override;

    void update_descriptor();



private:
    const VulkanDevice& device;
    image::Specification spec;

    Buffer image_data;

    VulkanImageInfo image_info;

    std::vector<vk::raii::ImageView> per_layer_image_views;
    std::unordered_map<size_t, vk::raii::ImageView> per_mip_image_views;
    vk::DescriptorImageInfo descriptor_image_info;
};

} // vulkan
