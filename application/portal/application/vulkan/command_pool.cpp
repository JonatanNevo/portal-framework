//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "command_pool.h"

#include "portal/application/vulkan/device.h"

namespace portal::vulkan
{
CommandPool::CommandPool(
    Device& device,
    uint32_t queue_family_index,
    rendering::RenderFrame* render_frame,
    const size_t thread_index,
    const CommandBuffer::ResetMode reset_mode
): VulkanResource(nullptr, &device),
   render_frame(render_frame),
   thread_index(thread_index),
   queue_family_index(queue_family_index),
   reset_mode(reset_mode)
{
    vk::CommandPoolCreateFlags flags;
    switch (reset_mode)
    {
    case CommandBuffer::ResetMode::ResetIndividually:
    case CommandBuffer::ResetMode::AlwaysAllocate:
        flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        break;
    default:
        flags = vk::CommandPoolCreateFlagBits::eTransient;
        break;
    }

    set_handle(device.get_handle().createCommandPool({flags, queue_family_index}));
}

CommandPool::CommandPool(CommandPool&& other) noexcept
    : VulkanResource(std::move(other)),
      render_frame(std::exchange(other.render_frame, nullptr)),
      thread_index(std::exchange(other.thread_index, 0)),
      queue_family_index(std::exchange(other.queue_family_index, 0)),
      primary_command_buffers(std::move(other.primary_command_buffers)),
      active_primary_command_buffer_count(std::exchange(other.active_primary_command_buffer_count, 0)),
      secondary_command_buffers(std::move(other.secondary_command_buffers)),
      active_secondary_command_buffer_count(std::exchange(other.active_secondary_command_buffer_count, 0)),
      reset_mode(std::exchange(other.reset_mode, {}))
{}

CommandPool::~CommandPool()
{
    // clear command buffers before destroying the command pool
    primary_command_buffers.clear();
    secondary_command_buffers.clear();

    // Destroy command pool
    if (has_handle())
        get_device_handle().destroyCommandPool(get_handle());
}

uint32_t CommandPool::get_queue_family_index() const
{
    return queue_family_index;
}

rendering::RenderFrame* CommandPool::get_render_frame() const
{
    return render_frame;
}

CommandBuffer::ResetMode CommandPool::get_reset_mode() const
{
    return reset_mode;
}

size_t CommandPool::get_thread_index() const
{
    return thread_index;
}

CommandBuffer& CommandPool::request_command_buffer(vk::CommandBufferLevel level)
{
    if (level == vk::CommandBufferLevel::ePrimary)
    {
        if (active_primary_command_buffer_count < primary_command_buffers.size())
            return *primary_command_buffers[active_primary_command_buffer_count++];

        primary_command_buffers.emplace_back(std::make_unique<CommandBuffer>(*this, level));
        active_primary_command_buffer_count++;
        return *primary_command_buffers.back();
    }
    else
    {
        if (active_secondary_command_buffer_count < secondary_command_buffers.size())
            return *secondary_command_buffers[active_secondary_command_buffer_count++];

        secondary_command_buffers.emplace_back(std::make_unique<CommandBuffer>(*this, level));
        active_secondary_command_buffer_count++;
        return *secondary_command_buffers.back();
    }
}

void CommandPool::reset_pool()
{
    switch (reset_mode)
    {
    case CommandBuffer::ResetMode::ResetPool:
        get_device_handle().resetCommandPool(get_handle());
        reset_command_buffers();
        break;
    case CommandBuffer::ResetMode::ResetIndividually:
        reset_command_buffers();
        break;
    case CommandBuffer::ResetMode::AlwaysAllocate:
        primary_command_buffers.clear();
        secondary_command_buffers.clear();
        active_primary_command_buffer_count = 0;
        active_secondary_command_buffer_count = 0;
        break;
    }
}

void CommandPool::reset_command_buffers()
{
    for (auto& cmd_buf : primary_command_buffers)
    {
        cmd_buf->reset(reset_mode);
    }
    active_primary_command_buffer_count = 0;

    for (auto& cmd_buf : secondary_command_buffers)
    {
        cmd_buf->reset(reset_mode);
    }
    active_secondary_command_buffer_count = 0;
}
} // portal
