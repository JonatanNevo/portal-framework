//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "vulkan_queue.h"

#include "portal/engine/renderer/vulkan/vulkan_device.h"

namespace portal::renderer::vulkan
{
VulkanQueue::VulkanQueue(
    const VulkanDevice& device,
    const size_t family_index,
    const vk::QueueFamilyProperties& properties,
    const size_t index,
    const bool presentable
)
    : queue(device.get_handle().getQueue(static_cast<uint32_t>(family_index), static_cast<uint32_t>(index))),
      family_index(family_index),
      index(index),
      properties(properties),
      presentable(presentable)
{
    //TODO: add debug name
}

void VulkanQueue::submit(const vk::SubmitInfo2& info, const vk::Fence fence) const
{
    queue.submit2(info, fence);
}

vk::Result VulkanQueue::present(const vk::PresentInfoKHR& info) const
{
    if (!presentable)
    {
        return vk::Result::eErrorIncompatibleDisplayKHR;
    }

    return queue.presentKHR(info);
}

size_t VulkanQueue::get_index() const
{
    return index;
}

size_t VulkanQueue::get_family_index() const
{
    return family_index;
}

bool VulkanQueue::is_presentable() const
{
    return presentable;
}

vk::QueueFamilyProperties VulkanQueue::get_properties() const
{
    return properties;
}

vk::Queue VulkanQueue::get_handle() const
{
    return queue;
}
} // portal
