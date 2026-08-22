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
 * @brief Inserts a barrier that only synchronizes access, without a layout transition
 * @param command_buffer Command buffer
 * @param image Image handle
 * @param subresource Subresource range
 * @param src_stage_mask Source pipeline stage
 * @param src_access_mask Source access mask
 * @param dst_stage_mask Destination pipeline stage
 * @param dst_access_mask Destination access mask
 * @param discard_contents If true, the old layout is set to Undefined, discarding the previous contents (e.g. first use after creation, or full overwrite); otherwise the old layout is General
 */
void image_barrier(
    const vk::CommandBuffer& command_buffer,
    const vk::Image& image,
    const vk::ImageSubresourceRange& subresource,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::AccessFlags2 src_access_mask,
    vk::PipelineStageFlags2 dst_stage_mask,
    vk::AccessFlags2 dst_access_mask,
    bool discard_contents = false);

/**
 * @brief Inserts a barrier that only synchronizes access, without a layout transition
 * @param command_buffer Command buffer
 * @param image Image
 * @param mip_count Mipmap count
 * @param src_stage_mask Source pipeline stage
 * @param src_access_mask Source access mask
 * @param dst_stage_mask Destination pipeline stage
 * @param dst_access_mask Destination access mask
 * @param discard_contents If true, the old layout is set to Undefined, discarding the previous contents (e.g. first use after creation, or full overwrite); otherwise the old layout is General
 * @param aspect_mask Image aspect (default: color)
 */
void image_barrier(
    const vk::CommandBuffer& command_buffer,
    const Reference<Image>& image,
    uint32_t mip_count,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::AccessFlags2 src_access_mask,
    vk::PipelineStageFlags2 dst_stage_mask,
    vk::AccessFlags2 dst_access_mask,
    bool discard_contents = false,
    vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor);
}
