//
// Created by Jonatan Nevo on 02/03/2025.
//

#include "queue.h"

#include "device.h"
#include "command_buffer.h"

namespace portal
{
vulkan::Queue::Queue(
    Device& device,
    const uint32_t family_index,
    const vk::QueueFamilyProperties& properties,
    const vk::Bool32 can_present,
    const uint32_t index
): device(device),
   family_index(family_index),
   index(index),
   can_present(can_present),
   properties(properties)
{
    handle = device.get_handle().getQueue(family_index, index);
}

vulkan::Queue::Queue(Queue&& other) noexcept :
    device(other.device),
    handle(std::exchange(other.handle, {})),
    family_index(std::exchange(other.family_index, {})),
    index(std::exchange(other.index, 0)),
    can_present(std::exchange(other.can_present, false)),
    properties(std::exchange(other.properties, {})) {}

void vulkan::Queue::submit(const CommandBuffer& command_buffer, const vk::Fence fence) const
{
    auto buffer_handle = command_buffer.get_handle();
    const vk::SubmitInfo submit_info({}, {}, buffer_handle);
    handle.submit(submit_info, fence);
}

vk::Result vulkan::Queue::present(const vk::PresentInfoKHR& present_infos) const
{
    if (!can_present)
        return vk::Result::eErrorIncompatibleDisplayKHR;
    return handle.presentKHR(present_infos);
}
} // portal
