//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//
#include "editor_module.h"

#include <imgui.h>
#include <imgui_internal.h>
#include "panel_manager.h"
#include "panels/details_panel.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/render_target/vulkan_render_target.h"
#include "portal/serialization/archive/json_archive.h"

namespace portal
{
EditorModule::EditorModule(
    ModuleStack& stack,
    renderer::vulkan::VulkanContext& context,
    renderer::vulkan::VulkanSwapchain& swapchain,
    const Window& window
)
    : TaggedModule(stack, STRING_ID("Editor Module")),
      swapchain(swapchain),
      runtime_module(stack, context, swapchain),
      im_gui_renderer(get_dependency<ResourcesModule>().get_registry(), window, swapchain),
      viewport(swapchain, runtime_module)
{
    panel_manager.add_panel<DetailsPanel>();
}

void EditorModule::begin_frame(FrameContext& frame)
{
    frame.rendering_context = swapchain.prepare_frame(frame);
    frame.active_scene = get_dependency<SystemOrchestrator>().get_active_scene();

    auto render_target = swapchain.get_current_render_target(false);
    im_gui_renderer.begin_frame(frame, render_target);
}

void EditorModule::gui_update(FrameContext& frame)
{
    panel_manager.on_gui_render(editor_context, frame);
    viewport.on_gui_update(frame);
}

void EditorModule::post_update(FrameContext& frame)
{
    viewport.render(frame);
}

void EditorModule::end_frame(FrameContext& frame)
{
    im_gui_renderer.end_frame(frame);

    swapchain.present(frame);
}

void EditorModule::on_event(Event&)
{
}
} // portal
