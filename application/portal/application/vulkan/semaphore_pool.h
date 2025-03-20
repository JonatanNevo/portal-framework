//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once

#include "portal/application/vulkan/common.h"

namespace portal::vulkan
{
class Device;

class SemaphorePool
{
public:
    explicit SemaphorePool(Device& device);
    ~SemaphorePool();

    SemaphorePool(const SemaphorePool&) = delete;
    SemaphorePool(SemaphorePool&& other) = delete;
    SemaphorePool& operator=(const SemaphorePool&) = delete;
    SemaphorePool& operator=(SemaphorePool&&) = delete;

    vk::Semaphore request_semaphore();
    vk::Semaphore request_semaphore_with_ownership();
    void release_owned_semaphore(vk::Semaphore semaphore);

    void reset();

    uint32_t get_active_semaphore_count() const;

private:
    Device& device;

    std::vector<vk::Semaphore> semaphores;
    std::vector<vk::Semaphore> released_semaphores;

    uint32_t active_semaphore_count{0};
};
} // portal
