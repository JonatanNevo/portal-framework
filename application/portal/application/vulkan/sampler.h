//
// Created by Jonatan Nevo on 03/03/2025.
//

#pragma once
#include "base/vulkan_resource.h"

namespace portal::vulkan
{

class Device;

class Sampler final : public VulkanResource<vk::Sampler>
{
public:
    Sampler(Device& device, const vk::SamplerCreateInfo& create_info);
    Sampler(Sampler&& other) noexcept;
    ~Sampler() override;

    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;
    Sampler& operator=(Sampler&& other) noexcept = delete;

};

} // portal
