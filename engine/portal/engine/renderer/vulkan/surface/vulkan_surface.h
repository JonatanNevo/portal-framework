//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/renderer/surface/surface.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"

namespace portal::renderer::vulkan
{

class VulkanSurface final : public Surface
{
public:
    explicit VulkanSurface(const VulkanContext& context, const SurfaceProperties& properties);

    [[nodiscard]] const SurfaceCapabilities& get_capabilities() const override;
    [[nodiscard]] glm::ivec2 get_extent() const override;

    [[nodiscard]] vk::SurfaceKHR get_vulkan_surface() const;

    [[nodiscard]] SurfaceType get_type() const override;

private:
    vk::raii::SurfaceKHR surface = nullptr;
    SurfaceCapabilities capabilities;
};

}
