//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace portal::vulkan
{

const std::vector g_validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector g_device_extensions = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName,
    vk::EXTCalibratedTimestampsExtensionName,
#if defined(PORTAL_PLATFORM_MACOS)
    vk::KHRPortabilitySubsetExtensionName
#endif
};


vk::SurfaceFormatKHR choose_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats);
vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes);
vk::Extent2D choose_extent(uint32_t width, uint32_t height, const vk::SurfaceCapabilitiesKHR& capabilities);

uint32_t rate_device_suitability(const vk::raii::PhysicalDevice& device);
uint32_t find_queue_families(const vk::raii::PhysicalDevice& device, vk::QueueFlagBits queue_type);

vk::SampleCountFlagBits get_max_usable_sample_count(vk::raii::PhysicalDevice& physical_device);

std::vector<const char*> get_required_extensions(const vk::raii::Context& context, bool enable_validation_layers);
std::vector<const char*> get_required_validation_layers(const vk::raii::Context& context, bool enable_validation_layers);

void transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::Image& image,
    uint32_t mip_level,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout
    );

void transition_image_layout(
    const vk::raii::CommandBuffer& command_buffer,
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
    const vk::raii::CommandBuffer& command_buffer,
    const vk::Image& image,
    const vk::ImageSubresourceRange& subresource,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask
    );

void copy_image_to_image(
    const vk::raii::CommandBuffer& command_buffer,
    const vk::Image& source,
    const vk::Image& dest,
    vk::Extent2D src_size,
    vk::Extent2D dst_size
    );
}
