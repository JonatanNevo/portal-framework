//
// Created by Jonatan Nevo on 02/03/2025.
//

#pragma once

#include <vulkan/vulkan.hpp>

namespace portal::vulkan
{
class CommandBuffer;
class Device;

class Queue
{
public:
    Queue(Device& device, uint32_t family_index, const vk::QueueFamilyProperties& properties, vk::Bool32 can_present, uint32_t index);
    Queue(Queue&& other) noexcept;
    Queue(const Queue&) = default;

    Queue& operator=(const Queue&) = delete;
    Queue& operator=(Queue&&) = delete;

    const Device& get_device() const { return device; }
    vk::Queue get_handle() const { return handle; }
    uint32_t get_family_index() const { return family_index; }
    uint32_t get_index() const { return index; }
    const vk::QueueFamilyProperties& get_properties() const { return properties; }
    vk::Bool32 support_present() const { return can_present; }

    void submit(const CommandBuffer& command_buffer, vk::Fence fence) const;
    vk::Result present(const vk::PresentInfoKHR& present_infos) const;

private:
    Device& device;
    vk::Queue handle;
    uint32_t family_index{0};
    uint32_t index{0};
    vk::Bool32 can_present = false;
    vk::QueueFamilyProperties properties{};
};
} // portal
