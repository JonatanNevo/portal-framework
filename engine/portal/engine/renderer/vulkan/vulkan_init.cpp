//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_init.h"

namespace portal::vulkan
{
vk::raii::CommandPool create_command_pool(
    const vk::raii::Device& device,
    const uint32_t queue_family_index,
    const vk::CommandPoolCreateFlags flags
    )
{
    const vk::CommandPoolCreateInfo create_info{
        .flags = flags,
        .queueFamilyIndex = queue_family_index
    };
    return device.createCommandPool(create_info);
}

vk::raii::CommandBuffer allocate_command_buffer(
    const vk::raii::Device& device,
    const vk::raii::CommandPool& command_pool,
    const vk::CommandBufferLevel level
    )
{
    return std::move(allocate_command_buffers(device, command_pool, 1, level).front());
}

std::vector<vk::raii::CommandBuffer> allocate_command_buffers(
    const vk::raii::Device& device,
    const vk::raii::CommandPool& command_pool,
    const uint32_t count,
    const vk::CommandBufferLevel level
    )
{
    const vk::CommandBufferAllocateInfo alloc_info{
        .commandPool = command_pool,
        .level = level,
        .commandBufferCount = count
    };
    return device.allocateCommandBuffers(alloc_info);
}

vk::raii::ImageView create_image_view(
    const vk::raii::Device& device,
    const vk::Image& image,
    uint32_t mip_level,
    const vk::Format format,
    const vk::ImageAspectFlags aspect_flags
    )
{
    const vk::ImageViewCreateInfo image_view_create_info = {
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {aspect_flags, 0, mip_level, 0, 1},
    };
    return device.createImageView(image_view_create_info);
}

}


