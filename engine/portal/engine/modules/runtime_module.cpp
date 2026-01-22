//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "runtime_module.h"

#include "resources_module.h"
#include "portal/engine/project/project.h"

namespace portal
{
RuntimeModule::RuntimeModule(
    ModuleStack& stack,
    Project& project,
    renderer::vulkan::VulkanContext& context,
    renderer::vulkan::VulkanSwapchain& swapchain
)
    : TaggedModule(stack, STRING_ID("Runtime Module")),
      project(project),
      swapchain(swapchain),
      renderer(project.get_settings(), context, get_dependency<ResourcesModule>().get_registry())
{}

RuntimeModule::~RuntimeModule()
{
    renderer.cleanup();
}

void RuntimeModule::begin_frame(FrameContext& frame)
{
    frame.rendering_context = swapchain.prepare_frame(frame);
    frame.active_scene = get_dependency<SystemOrchestrator>().get_active_scene();
}

void RuntimeModule::post_update(FrameContext& frame)
{
    inner_post_update(frame, swapchain.get_current_render_target());
}

void RuntimeModule::end_frame(FrameContext& frame)
{
    inner_end_frame(frame, true);
}

void RuntimeModule::on_event(Event&)
{
}

void RuntimeModule::inner_post_update(FrameContext& frame, const Reference<renderer::RenderTarget>& render_target)
{
    renderer.begin_frame(frame, render_target);
    renderer.post_update(frame);
}

void RuntimeModule::inner_end_frame(FrameContext& frame, const bool present)
{
    renderer.end_frame(frame);

    if (present)
        swapchain.present(frame);
}
} // portal
