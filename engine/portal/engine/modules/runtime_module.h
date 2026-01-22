//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "system_orchestrator.h"
#include "resources_module.h"
#include "portal/application/modules/module.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/renderer/vulkan/vulkan_swapchain.h"

namespace portal
{

/**
 * @brief Module responsible for runtime scene rendering.
 *
 * RuntimeModule handles the core rendering loop including frame lifecycle management,
 * scene rendering, and swapchain presentation. It provides both standard rendering
 * (directly to swapchain) and inner methods for rendering to custom render targets
 * (used by the editor viewport).
 */
class RuntimeModule final: public TaggedModule<
        Tag<
            ModuleTags::FrameLifecycle,
            ModuleTags::PostUpdate,
            ModuleTags::Event
        >,
        SystemOrchestrator,
        ResourcesModule
    >
{
public:
    /**
     * @brief Constructs the runtime module.
     * @param stack The module stack this module belongs to.
     * @param context The Vulkan context.
     * @param swapchain The swapchain for presentation.
     */
    RuntimeModule(
        ModuleStack& stack,
        Project& project,
        renderer::vulkan::VulkanContext& context,
        renderer::vulkan::VulkanSwapchain& swapchain
    );

    ~RuntimeModule() override;

    void begin_frame(FrameContext& frame) override;
    void post_update(FrameContext& frame) override;
    void end_frame(FrameContext& frame) override;
    void on_event(Event& event) override;

    /**
     * @brief Renders the scene to a custom render target.
     *
     * Used by the editor viewport to render the scene to an offscreen target
     * instead of the swapchain.
     * @param frame The current frame context.
     * @param render_target The target to render to.
     */
    void inner_post_update(FrameContext& frame, const Reference<renderer::RenderTarget>& render_target);

    /**
     * @brief Completes frame rendering with optional presentation.
     * @param frame The current frame context.
     * @param present If true, presents to the swapchain. Set to false when
     *        rendering to a custom target.
     */
    void inner_end_frame(FrameContext& frame, bool present = true);

private:
    [[maybe_unused]] Project& project;
    renderer::vulkan::VulkanSwapchain& swapchain;

    Renderer renderer;
};

} // portal
