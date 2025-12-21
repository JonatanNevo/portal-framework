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

class Renderer final : public TaggedModule<Tag<ModuleTags::FrameLifecycle, ModuleTags::Update, ModuleTags::PostUpdate>>
{
public:
    Renderer(ModuleStack& stack, renderer::vulkan::VulkanContext& context);
    ~Renderer() override;

    void set_swapchain(const Reference<renderer::vulkan::VulkanSwapchain>& new_swapchain);
    void cleanup();

    void begin_frame(FrameContext& frame) override;
    void end_frame(FrameContext& frame) override;

    void post_update(FrameContext& frame) override;
    void draw_geometry(FrameContext& frame, const vk::raii::CommandBuffer& command_buffer);

    void on_resize(size_t new_width, size_t new_height) const;

    [[nodiscard]] const RendererContext& get_renderer_context() const;
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
