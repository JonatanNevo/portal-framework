//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "portal/engine/renderer/deletion_queue.h"
#include "portal/engine/renderer/renderer_context.h"
#include "portal/engine/renderer/vulkan/vulkan_context.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"
#include "portal/engine/renderer/rendering_context.h"
#include "portal/engine/renderer/vulkan/image/vulkan_image.h"
#include "portal/engine/scene/scene.h"
#include "vulkan/descriptors/vulkan_descriptor_set_manager.h"

namespace portal
{
namespace renderer
{
    class DescriptorSetManager;
    class UniformBufferSet;
    class RenderTarget;
}

namespace renderer::vulkan
{
    class VulkanPipeline;
    class VulkanWindow;
}

class Window;

/**
 * @class Renderer
 * @brief Main renderer with N-frames-in-flight synchronization
 *
 * Manages frame pacing, render targets, and per-frame resources.
 *
 * ## Frame Synchronization Flow
 *
 * begin_frame():
 * 1. Wait for current frame's fence (ensures GPU finished this frame index)
 * 2. Acquire next swapchain image (wait for image_available_semaphore)
 * 3. If swapchain image was used by previous frame, wait for that frame's fence
 * 4. Reset command pool and descriptor allocator
 * 5. Begin command buffer recording
 *
 * end_frame():
 * 1. End command buffer recording
 * 2. Submit to graphics queue (wait on image_available, signal render_finished)
 * 3. Present swapchain image (wait on render_finished)
 * 4. Advance current_frame index (mod frames_in_flight)
 */
class Renderer
{
public:
    /**
     * @brief Constructs renderer
     * @param stack Module stack
     * @param context Vulkan context
     */
    Renderer(ProjectSettings& settings, renderer::vulkan::VulkanContext& context, ResourceRegistry& resource_registry);

    /** @brief Destructor (cleans up frame resources) */
    ~Renderer();

    /** @brief Cleans up renderer resources */
    void cleanup();

    /**
     * @brief Begins frame (wait for fence, acquire image, reset pools)
     * @param frame Frame context
     */
    void begin_frame(const FrameContext& frame, const Reference<renderer::RenderTarget>& render_target);

    /**
     * @brief Ends frame (submit commands, present)
     * @param frame Frame context
     */
    void end_frame(FrameContext& frame);

    /**
     * @brief Post-update hook (renders geometry)
     * @param frame Frame context
     */
    void post_update(FrameContext& frame);

    /**
     * @brief Records geometry rendering commands
     * @param frame Frame context
     * @param command_buffer Command buffer to record into
     */
    void draw_geometry(FrameContext& frame, const vk::CommandBuffer& command_buffer);


    /** @brief Gets renderer context */
    [[nodiscard]] const RendererContext& get_renderer_context() const;

private:
    void immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function);

    void init_global_descriptors(ResourceRegistry& resource_registry);

private:
    ProjectSettings& settings;
    renderer::vulkan::VulkanContext& context;

    Reference<renderer::RenderTarget> current_render_target;
    Reference<renderer::Image> current_draw_image;
    Reference<renderer::Image> current_depth_image;


    // TODO: Where should these be?
    Reference<renderer::UniformBufferSet> scene_data_uniform_buffer;
    Reference<renderer::UniformBufferSet> scene_lights_uniform_buffer;

    std::unique_ptr<renderer::vulkan::VulkanDescriptorSetManager> descriptor_set_manager;

    DeletionQueue deletion_queue = {};

    vk::raii::Fence immediate_fence = nullptr;
    vk::raii::CommandPool immediate_command_pool = nullptr;
    vk::raii::CommandBuffer immediate_command_buffer = nullptr;

    RendererContext renderer_context;
    bool is_initialized = false;
};
} // portal
