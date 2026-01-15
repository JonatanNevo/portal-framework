//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/renderer/image/image.h"


namespace portal::renderer::vulkan
{
/**
 * @brief Gets maximum usable MSAA sample count
 * @param physical_device Physical device
 * @return Maximum sample count
 */
vk::SampleCountFlagBits get_max_usable_sample_count(vk::raii::PhysicalDevice& physical_device);

/**
 * @brief Transitions image layout (simple version)
 * @param command_buffer Command buffer
 * @param image Image handle
 * @param mip_level Mipmap level
 * @param old_layout Source layout
 * @param new_layout Destination layout
 */
void transition_image_layout(
    const vk::CommandBuffer& command_buffer,
    const vk::Image& image,
    uint32_t mip_level,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout
);

void transition_image_layout(
    const vk::CommandBuffer& command_buffer,
    const Reference<Image>& image,
    uint32_t mip_level,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout
);

/**
 * @brief Transitions image layout with explicit synchronization
 * @param command_buffer Command buffer
 * @param image Image handle
 * @param mip_level Mipmap level
 * @param old_layout Source layout
 * @param new_layout Destination layout
 * @param src_access_mask Source access mask
 * @param dst_access_mask Destination access mask
 * @param src_stage_mask Source pipeline stage
 * @param dst_stage_mask Destination pipeline stage
 * @param aspect_mask Image aspect (default: color)
 */
void transition_image_layout(
    const vk::CommandBuffer& command_buffer,
    const vk::Image& image,
    uint32_t mip_level,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask,
    vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor
);

void transition_image_layout(
    const vk::CommandBuffer& command_buffer,
    const Reference<Image>& image,
    uint32_t mip_level,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask,
    vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor
);

/**
 * @brief Transitions image layout with subresource range
 * @param command_buffer Command buffer
 * @param image Image handle
 * @param subresource Subresource range
 * @param old_layout Source layout
 * @param new_layout Destination layout
 * @param src_access_mask Source access mask
 * @param dst_access_mask Destination access mask
 * @param src_stage_mask Source pipeline stage
 * @param dst_stage_mask Destination pipeline stage
 */
void transition_image_layout(
    const vk::CommandBuffer& command_buffer,
    const vk::Image& image,
    const vk::ImageSubresourceRange& subresource,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask
);

void transition_image_layout(
    const vk::CommandBuffer& command_buffer,
    const Reference<Image>& image,
    const vk::ImageSubresourceRange& subresource,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask
);

/**
 * @brief Copies image to image with blit
 * @param command_buffer Command buffer
 * @param source Source image
 * @param dest Destination image
 * @param src_size Source extent
 * @param dst_size Destination extent
 */
void copy_image_to_image(
    const vk::CommandBuffer& command_buffer,
    const vk::Image& source,
    const vk::Image& dest,
    vk::Extent2D src_size,
    vk::Extent2D dst_size
);
}
