//
// Created by Jonatan Nevo on 02/03/2025.
//

#pragma once

#include "base/vulkan_resource.h"
#include "physical_device.h"
#include "debug_utils.h"
#include "queue.h"
#include "portal/application/vulkan/command_pool.h"
#include "portal/application/vulkan/resources/resource_cache.h"
#include "portal/application/vulkan/fence_pool.h"

namespace portal::vulkan
{
class Buffer;

class Device final : public VulkanResource<vk::Device>
{
public:
    /**
     * @brief Device constructor
     * @param gpu A valid Vulkan physical device and the requested gpu features
     * @param surface The surface
     * @param debug_utils The debug utils to be associated to this device
     * @param requested_extensions (Optional) List of required device extensions and whether support is optional or not
     */
    Device(
        PhysicalDevice& gpu,
        vk::SurfaceKHR surface,
        std::unique_ptr<DebugUtils>&& debug_utils,
        std::unordered_map<const char*, bool> requested_extensions = {}
    );

    ~Device() override;

    Device(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) = delete;

    const PhysicalDevice& get_gpu() const;
    const DebugUtils& get_debug_utils() const;

    const Queue& get_queue(uint32_t queue_family_index, uint32_t queue_index) const;
    const Queue& get_queue_by_flags(vk::QueueFlags queue_flags, uint32_t queue_index) const;
    const Queue& get_queue_by_present(uint32_t queue_index) const;

    /**
     * @brief Finds a suitable graphics queue to submit to
     * @return The first present supported queue, otherwise just any graphics queue
     */
    const Queue& get_suitable_graphics_queue() const;

    bool is_extension_supported(const std::string& extension) const;
    bool is_enabled(const std::string& extension) const;
    uint32_t get_queue_family_index(vk::QueueFlagBits queue_flag) const;

    CommandPool& get_command_pool() const;

    /**
     * @brief Creates a vulkan image and associated device memory
     * @param format The image format
     * @param extent The image extent
     * @param mip_levels The mip levels of the image
     * @param usage The image usage
     * @param properties The device memory property flags
     * @returns A valid vk::Image and a corresponding vk::DeviceMemory
     */
    [[nodiscard]] std::pair<vk::Image, vk::DeviceMemory> create_image(
        vk::Format format,
        vk::Extent2D const& extent,
        uint32_t mip_levels,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties
    ) const;

    /**
     * @brief Copies a buffer from one to another
     * @param src The buffer to copy from
     * @param dst The buffer to copy to
     * @param queue The queue to submit the copy command to
     * @param copy_region The amount to copy, if null copies the entire buffer
     */
    void copy_buffer(Buffer& src, Buffer& dst, vk::Queue queue, const vk::BufferCopy* copy_region = nullptr) const;

    /**
     * @brief Requests a command buffer from the device's command pool
     * @param level The command buffer level
     * @param begin Whether the command buffer should be implicitly started before it's returned
     * @returns A valid vk::CommandBuffer
     */
    vk::CommandBuffer create_command_buffer(vk::CommandBufferLevel level, bool begin = false) const;

    /**
     * @brief Submits and frees up a given command buffer
     * @param command_buffer The command buffer
     * @param queue The queue to submit the work to
     * @param free Whether the command buffer should be implicitly freed up
     * @param signalSemaphore An optional semaphore to signal when the commands have been executed
     */
    void flush_command_buffer(
        vk::CommandBuffer command_buffer,
        vk::Queue queue,
        bool free = true,
        vk::Semaphore signalSemaphore = VK_NULL_HANDLE
    ) const;

    FencePool& get_fence_pool() const;

    ResourceCache& get_resource_cache();

private:
    const PhysicalDevice& gpu;
    vk::SurfaceKHR surface = nullptr;
    std::unique_ptr<DebugUtils> debug_utils;
    std::vector<const char*> enabled_extensions{};
    std::vector<std::vector<Queue>> queues{};

    std::unique_ptr<CommandPool> command_pool;
    std::unique_ptr<FencePool> fence_pool;
    ResourceCache resource_cache;
};
} // portal
