//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "editor_context.h"
#include "panel_manager.h"
#include "viewport.h"
#include "portal/application/modules/module.h"
#include "portal/engine/imgui/imgui_renderer.h"
#include "portal/engine/modules/resources_module.h"
#include "portal/engine/modules/runtime_module.h"
#include "portal/engine/renderer/renderer.h"

namespace portal
{

/**
 * @brief Top-level module for the editor application.
 *
 * EditorModule orchestrates the editor UI and scene rendering by combining:
 * - RuntimeModule for scene rendering to the viewport
 * - ImGuiRenderer for the editor UI
 * - Viewport for displaying the rendered scene in an ImGui window
 *
 * Unlike RuntimeModule which renders directly to the swapchain, EditorModule
 * renders the scene to a viewport texture and composites it with the editor UI.
 */
class EditorModule final : public TaggedModule<
        Tag<
            ModuleTags::FrameLifecycle,
            ModuleTags::PostUpdate,
            ModuleTags::GuiUpdate,
            ModuleTags::Event
        >,
        SystemOrchestrator,
        ResourcesModule
    >
{
public:
    EditorModule(
        ModuleStack& stack,
        renderer::vulkan::VulkanContext& context,
        renderer::vulkan::VulkanSwapchain& swapchain,
        const Window& window
    );

    void begin_frame(FrameContext& frame) override;
    void gui_update(FrameContext& frame) override;
    void post_update(FrameContext& frame) override;
    void end_frame(FrameContext& frame) override;
    void on_event(Event& event) override;

private:
    const Window& window;
    renderer::vulkan::VulkanSwapchain& swapchain;
    RuntimeModule runtime_module;
    ImGuiRenderer im_gui_renderer;
    PanelManager panel_manager;

    EditorContext editor_context;
    Viewport viewport;
};
} // portal
