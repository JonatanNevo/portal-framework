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

/**
 * @class VulkanSampler
 * @brief Vulkan sampler with wrap/filter configuration
 */
class VulkanSampler final : public Sampler
{
public:
    /**
     * @brief Constructs Vulkan sampler
     * @param id Sampler ID
     * @param properties Sampler properties
     * @param device Vulkan device
     */
    VulkanSampler(const StringId& id, const SamplerProperties& properties, const VulkanDevice& device);

    /** @brief Gets Vulkan sampler handle */
    vk::Sampler get_vk_sampler() const;

    /** @brief Gets sampler properties */
    [[nodiscard]] const SamplerProperties& get_prop() const override;

private:
    StringId id;
    SamplerProperties properties;
    vk::raii::Sampler sampler = nullptr;
};
} // portal
