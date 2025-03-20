//
// Created by Jonatan Nevo on 03/03/2025.
//

#include "sampler.h"

#include "portal/application/vulkan/device.h"

namespace portal::vulkan
{
Sampler::Sampler(Device& device, const vk::SamplerCreateInfo& create_info)
    : VulkanResource(device.get_handle().createSampler(create_info), &device)
{}

Sampler::Sampler(Sampler&& other) noexcept: VulkanResource(std::move(other)) {}

Sampler::~Sampler()
{
    if (get_handle())
        get_device().get_handle().destroySampler(get_handle());
}
} // portal
