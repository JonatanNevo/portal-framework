//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/renderer/image/image_types.h"
#include "portal/engine/renderer/image/sampler.h"

namespace portal::renderer::vulkan
{
class VulkanDevice;

class VulkanSampler final : public Sampler
{
public:
    VulkanSampler(const StringId& id, const SamplerProperties& properties, const VulkanDevice& device);

    vk::Sampler get_vk_sampler() const;
    [[nodiscard]] const SamplerProperties& get_prop() const override;

private:
    StringId id;
    SamplerProperties properties;
    vk::raii::Sampler sampler = nullptr;
};
} // portal
