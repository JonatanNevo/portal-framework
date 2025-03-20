//
// Created by Jonatan Nevo on 04/03/2025.
//

#pragma once

#include "portal/application/vulkan/common.h"

namespace portal::vulkan
{

class Device;

class FencePool
{
public:
    explicit FencePool(Device& device);
    ~FencePool();

    FencePool(const FencePool &) = delete;
    FencePool(FencePool &&other) = delete;

    vk::Fence request_fence();
    vk::Result wait(uint32_t timeout = std::numeric_limits<uint32_t>::max());
    vk::Result reset();

private:
    Device& device;
    std::vector<vk::Fence> fences;
    uint32_t active_fence_count = 0;
};

}
