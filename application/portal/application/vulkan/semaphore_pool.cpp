//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "semaphore_pool.h"

#include "portal/application/vulkan/device.h"

namespace portal::vulkan
{
SemaphorePool::SemaphorePool(Device& device): device(device) {}

SemaphorePool::~SemaphorePool()
{
    reset();

    for (const auto& semaphore : semaphores)
        device.get_handle().destroySemaphore(semaphore);

    semaphores.clear();
}

vk::Semaphore SemaphorePool::request_semaphore()
{
    // Check if there is an available semaphore
    if (active_semaphore_count < semaphores.size())
        return semaphores[active_semaphore_count++];

    const auto semaphore = device.get_handle().createSemaphore({});
    semaphores.push_back(semaphore);
    active_semaphore_count++;
    return semaphore;
}

vk::Semaphore SemaphorePool::request_semaphore_with_ownership()
{
    // Check if there is an available semaphore, if so, just pilfer one.
    if (active_semaphore_count < semaphores.size())
    {
        const auto semaphore = semaphores.back();
        semaphores.pop_back();
        return semaphore;
    }
    return device.get_handle().createSemaphore({});
}

void SemaphorePool::release_owned_semaphore(vk::Semaphore semaphore)
{
    // We cannot reuse this semaphore until ::reset().
    released_semaphores.push_back(semaphore);
}

void SemaphorePool::reset()
{
    active_semaphore_count = 0;

    // Now we can safely recycle the released semaphores.
    for (auto& sem : released_semaphores)
    {
        semaphores.push_back(sem);
    }

    released_semaphores.clear();
}

uint32_t SemaphorePool::get_active_semaphore_count() const
{
    return active_semaphore_count;
}
} // portal
