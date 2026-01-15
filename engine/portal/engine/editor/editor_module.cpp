//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "editor_module.h"

#include "portal/engine/renderer/descriptor_layout_builder.h"

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
      context(context),
      viewport_renderer(context, get_dependency<ResourceRegistry>()),
      im_gui_renderer(window, swapchain)
{}

EditorModule::~EditorModule()
{
    viewport_renderer.cleanup();
}

void EditorModule::begin_frame(FrameContext& frame)
{
    frame.rendering_context = swapchain.prepare_frame(frame);

    im_gui_renderer.begin_frame(frame);
    viewport_renderer.begin_frame(frame);
}


void EditorModule::post_update(FrameContext& frame)
{
    viewport_renderer.post_update(frame);
}

void EditorModule::end_frame(FrameContext& frame)
{
    viewport_renderer.end_frame(frame);
    im_gui_renderer.end_frame(frame);

    swapchain.present(frame);
}

void EditorModule::gui_update(FrameContext& frame)
{
    gui_system.execute(*frame.ecs_registry);
    im_gui_renderer.gui_update(frame);
}

void EditorModule::on_event(Event&)
{
}


} // portal
