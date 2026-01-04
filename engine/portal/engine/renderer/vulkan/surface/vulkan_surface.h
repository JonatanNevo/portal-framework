//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/renderer/surface/surface.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"

namespace portal::renderer::vulkan
{
/**
 * @class VulkanSurface
 * @brief Vulkan presentation surface
 *
 * Creates vk::SurfaceKHR and queries surface capabilities for swapchain creation.
 */
class VulkanSurface final : public Surface
{
public:
    /**
     * @brief Constructs Vulkan surface
     * @param context Vulkan context
     * @param properties Surface configuration
     */
    explicit VulkanSurface(const VulkanContext& context, const SurfaceProperties& properties);

    /** @brief Gets surface capabilities */
    [[nodiscard]] const SurfaceCapabilities& get_capabilities() const override;

    /** @brief Gets surface extent */
    [[nodiscard]] glm::ivec2 get_extent() const override;

    /** @brief Gets Vulkan surface handle */
    [[nodiscard]] vk::SurfaceKHR get_vulkan_surface() const;

    /** @brief Gets surface type */
    [[nodiscard]] SurfaceType get_type() const override;

    /** @brief Gets minimum frames in flight */
    [[nodiscard]] size_t get_min_frames_in_flight() const override;

private:
    vk::raii::SurfaceKHR surface = nullptr;
    SurfaceCapabilities capabilities;
};
}
