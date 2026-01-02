//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "portal/engine/renderer/queue/queue.h"

namespace portal::renderer::vulkan
{
class VulkanDevice;

/**
 * @class VulkanQueue
 * @brief Vulkan command queue for submitting work and presenting images
 *
 * Wraps vk::raii::Queue with queue family information and presentation capability.
 * Queues are retrieved from VulkanDevice during device creation.
 */
class VulkanQueue final : public Queue
{
public:
    /**
     * @brief Constructs Vulkan queue
     * @param device Vulkan device
     * @param family_index Queue family index
     * @param properties Queue family properties
     * @param index Queue index within family
     * @param presentable Whether queue supports presentation
     */
    VulkanQueue(const VulkanDevice& device, size_t family_index, const vk::QueueFamilyProperties& properties, size_t index, bool presentable);

    /**
     * @brief Submits commands to queue
     * @param info Submit info (command buffers, semaphores, etc.)
     * @param fence Fence to signal when submission completes
     */
    // TODO: use generic type
    void submit(const vk::SubmitInfo2& info, vk::Fence fence) const;

    /**
     * @brief Presents swapchain image
     * @param info Present info (swapchain, image index, semaphores)
     * @return Result (success, suboptimal, out of date, etc.)
     */
    vk::Result present(const vk::PresentInfoKHR& info) const;

    /** @brief Gets queue index within family */
    [[nodiscard]] size_t get_index() const override;

    /** @brief Gets queue family index */
    [[nodiscard]] size_t get_family_index() const;

    /** @brief Checks if queue supports presentation */
    [[nodiscard]] bool is_presentable() const;

    /** @brief Gets queue family properties */
    [[nodiscard]] vk::QueueFamilyProperties get_properties() const;

    /** @brief Gets queue handle */
    [[nodiscard]] vk::Queue get_handle() const;

private:
    vk::raii::Queue queue;
    size_t family_index;
    size_t index;

    vk::QueueFamilyProperties properties;
    bool presentable;
};
} // portal
