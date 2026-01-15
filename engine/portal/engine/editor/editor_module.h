//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "editor_system.h"
#include "portal/application/modules/module.h"
#include "portal/engine/imgui/imgui_renderer.h"
#include "portal/engine/renderer/renderer.h"
#include "portal/engine/renderer/descriptors/descriptor_set_manager.h"

namespace portal
{
class EditorModule final : public TaggedModule<
        Tag<
            ModuleTags::FrameLifecycle,
            ModuleTags::PostUpdate,
            ModuleTags::GuiUpdate,
            ModuleTags::Event
        >,
        SystemOrchestrator,
        ResourceRegistry
    >
{
public:
    EditorModule(
        ModuleStack& stack,
        renderer::vulkan::VulkanContext& context,
        renderer::vulkan::VulkanSwapchain& swapchain,
        const Window& window
    );
    ~EditorModule() override;

    void begin_frame(FrameContext& frame) override;
    void gui_update(FrameContext& frame) override;
    void post_update(FrameContext& frame) override;
    void end_frame(FrameContext& frame) override;
    void on_event(Event& event) override;


private:
    renderer::vulkan::VulkanSwapchain& swapchain;
    renderer::vulkan::VulkanContext& context;

    vk::DescriptorSet viewport_descriptor_set;
    Reference<renderer::RenderTarget> viewport_render_target;

    Renderer viewport_renderer;
    ImGuiRenderer im_gui_renderer;
    EditorGuiSystem gui_system;
};
} // portal
