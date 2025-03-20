//
// Created by Jonatan Nevo on 04/03/2025.
//

#include "fence_pool.h"
#include "portal/application/vulkan/device.h"

namespace portal::vulkan
{
FencePool::FencePool(Device& device): device(device) {}

FencePool::~FencePool()
{
    wait();
    reset();

    for (const auto& fence : fences)
        device.get_handle().destroyFence(fence);

    fences.clear();
}

vk::Fence FencePool::request_fence()
{
    // Check if there is an available fence
    if (active_fence_count < fences.size())
    {
        return fences[active_fence_count++];
    }

    const vk::Fence fence = device.get_handle().createFence({});
    if (!fence)
        throw std::runtime_error("failed to create fence");

    fences.push_back(fence);
    active_fence_count++;
    return fence;
}

vk::Result FencePool::wait(const uint32_t timeout)
{
    if (active_fence_count < 1 || fences.empty())
        return vk::Result::eSuccess;

    return device.get_handle().waitForFences(fences, VK_TRUE, timeout);
}

vk::Result FencePool::reset()
{
    if (active_fence_count < 1 || fences.empty())
        return vk::Result::eSuccess;

    const auto result = device.get_handle().resetFences(fences.size(), fences.data());
    if (result != vk::Result::eSuccess)
        return result;

    active_fence_count = 0;
    return vk::Result::eSuccess;
}
}
