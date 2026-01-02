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

namespace portal
{
namespace renderer
{
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
 * Manages frame pacing, swapchain, render targets, and per-frame resources.
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
class Renderer final : public TaggedModule<Tag<ModuleTags::FrameLifecycle, ModuleTags::Update, ModuleTags::PostUpdate>>
{
public:
    /**
     * @brief Constructs renderer
     * @param stack Module stack
     * @param context Vulkan context
     */
    Renderer(ModuleStack& stack, renderer::vulkan::VulkanContext& context);

    /** @brief Destructor (cleans up frame resources) */
    ~Renderer() override;

    /**
     * @brief Sets swapchain reference
     * @param new_swapchain Swapchain to use for presentation
     */
    void set_swapchain(const Reference<renderer::vulkan::VulkanSwapchain>& new_swapchain);

    /** @brief Cleans up renderer resources */
    void cleanup();

    /**
     * @brief Begins frame (wait for fence, acquire image, reset pools)
     * @param frame Frame context
     */
    void begin_frame(FrameContext& frame) override;

    /**
     * @brief Ends frame (submit commands, present)
     * @param frame Frame context
     */
    void end_frame(FrameContext& frame) override;

    /**
     * @brief Post-update hook (renders geometry)
     * @param frame Frame context
     */
    void post_update(FrameContext& frame) override;

    /**
     * @brief Records geometry rendering commands
     * @param frame Frame context
     * @param command_buffer Command buffer to record into
     */
    void draw_geometry(FrameContext& frame, const vk::raii::CommandBuffer& command_buffer);

    /**
     * @brief Handles swapchain resize
     * @param new_width New width
     * @param new_height New height
     */
    void on_resize(size_t new_width, size_t new_height) const;

    /** @brief Gets renderer context */
    [[nodiscard]] const RendererContext& get_renderer_context() const;

    /** @brief Gets swapchain */
    [[nodiscard]] const renderer::vulkan::VulkanSwapchain& get_swapchain() const;

private:
    void init_render_target();
    void init_frame_resources();

    void immediate_submit(std::function<void(vk::raii::CommandBuffer&)>&& function);

    static void clean_frame(const FrameContext& frame);

private:
    Reference<renderer::vulkan::VulkanSwapchain> swapchain;
    renderer::AttachmentProperties attachments;
    renderer::vulkan::VulkanContext& context;

    // TODO: move from here to some `g buffer` class
    Reference<renderer::vulkan::VulkanImage> depth_image;
    Reference<renderer::vulkan::VulkanRenderTarget> render_target;

    DeletionQueue deletion_queue = {};

    vk::raii::Fence immediate_fence = nullptr;
    vk::raii::CommandPool immediate_command_pool = nullptr;
    vk::raii::CommandBuffer immediate_command_buffer = nullptr;

    std::vector<vk::raii::DescriptorSetLayout> scene_descriptor_set_layouts;
    std::vector<renderer::FrameResources> frames;

    RendererContext renderer_context;
    bool is_initialized = false;

    size_t frames_in_flight = 0;

    // Index of the frame we are currently working on, up to max frames in flight
    size_t current_frame = 0;
};
} // portal
