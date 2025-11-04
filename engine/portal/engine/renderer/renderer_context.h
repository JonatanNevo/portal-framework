//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/engine/renderer/render_target/render_target.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"

#include <ranges>

namespace portal
{

class RendererContext
{
public:
    RendererContext(
        renderer::vulkan::VulkanContext& gpu_context,
        Reference<renderer::RenderTarget>& render_target,
        std::vector<vk::raii::DescriptorSetLayout>& global_descriptor_set_layout
        );


    [[nodiscard]] const renderer::vulkan::VulkanContext& get_gpu_context() const;
    [[nodiscard]] renderer::vulkan::VulkanContext& get_gpu_context();

    [[nodiscard]] inline auto get_global_descriptor_set_layout() const
    {
        return global_descriptor_set_layout | std::ranges::views::transform([](const auto& layout) { return *layout; });
    }

    [[nodiscard]] const Reference<renderer::RenderTarget>& get_render_target() const;
    [[nodiscard]] Reference<renderer::RenderTarget>& get_render_target();

protected:
    // TODO: use baseclasses here
    renderer::vulkan::VulkanContext& gpu_context;
    Reference<renderer::RenderTarget>& render_target;
    std::vector<vk::raii::DescriptorSetLayout>& global_descriptor_set_layout;
};

} // portal