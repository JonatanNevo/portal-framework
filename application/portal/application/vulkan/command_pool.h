//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once
#include "portal/application/vulkan/command_buffer.h"

namespace portal::vulkan
{
namespace rendering {
    class RenderFrame;
}

#include <cstdint>

class Device;
class RenderFrame;

class CommandPool final : public VulkanResource<vk::CommandPool>
{
public:
    CommandPool(
        Device& device,
        uint32_t queue_family_index,
        rendering::RenderFrame* render_frame = nullptr,
        size_t thread_index = 0,
        CommandBuffer::ResetMode reset_mode = CommandBuffer::ResetMode::ResetPool
    );
    CommandPool(CommandPool&& other) noexcept;
    ~CommandPool() override;

    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool& operator=(CommandPool&&) = delete;

    [[nodiscard]] uint32_t get_queue_family_index() const;
    rendering::RenderFrame* get_render_frame() const;
    CommandBuffer::ResetMode get_reset_mode() const;
    size_t get_thread_index() const;
    CommandBuffer& request_command_buffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);
    void reset_pool();

private:
    void reset_command_buffers();

private:
    rendering::RenderFrame* render_frame = nullptr;
    size_t thread_index = 0;
    uint32_t queue_family_index = 0;
    std::vector<std::unique_ptr<CommandBuffer>> primary_command_buffers;
    uint32_t active_primary_command_buffer_count = 0;
    std::vector<std::unique_ptr<CommandBuffer>> secondary_command_buffers;
    uint32_t active_secondary_command_buffer_count = 0;
    CommandBuffer::ResetMode reset_mode = CommandBuffer::ResetMode::ResetPool;
};
} // portal
