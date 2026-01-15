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
    RuntimeModule(
        ModuleStack& stack,
        renderer::vulkan::VulkanContext& context,
        renderer::vulkan::VulkanSwapchain& swapchain
    );

    ~RuntimeModule() override;

    void begin_frame(FrameContext& frame) override;
    void post_update(FrameContext& frame) override;
    void end_frame(FrameContext& frame) override;
    void on_event(Event& event) override;

    void inner_post_update(FrameContext& frame, const Reference<renderer::RenderTarget>& render_target);
    void inner_end_frame(FrameContext& frame, bool present = true);

private:
    renderer::vulkan::VulkanSwapchain& swapchain;
    renderer::vulkan::VulkanContext& context;

    Renderer renderer;
};

} // portal
