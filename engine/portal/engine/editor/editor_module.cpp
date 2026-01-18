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
#include "portal/engine/window/glfw_window.h"
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
      window(window),
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
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    io.ConfigWindowsResizeFromEdges = io.BackendFlags & ImGuiBackendFlags_HasMouseCursors;


    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    const ImGuiViewport* imgui_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(imgui_viewport->Pos);
    ImGui::SetNextWindowSize(imgui_viewport->Size);
    ImGui::SetNextWindowViewport(imgui_viewport->ID);


    // TODO: this can be a generic function on the window class
    const auto& vulkan_window = dynamic_cast<const GlfwWindow&>(window);
    bool is_maximized = glfwGetWindowAttrib(vulkan_window.get_handle(), GLFW_MAXIMIZED) == GLFW_TRUE;

#ifdef PORTAL_PLATFORM_WINDOWS
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, is_maximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);
#else
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
#endif

    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});
    ImGui::Begin("Main Window", nullptr, window_flags);
    ImGui::PopStyleColor(); // MenuBarBg
    ImGui::PopStyleVar(2);

    const auto min_win_size = style.WindowMinSize;
    style.WindowMinSize.x = 325.0f;
    style.WindowMinSize.y = 150.0f;
    ImGui::DockSpace(ImGui::GetID("Editor"));
    style.WindowMinSize = min_win_size;


    panel_manager.on_gui_render(editor_context, frame);
    viewport.on_gui_update(frame);

    ImGui::End();
}

void EditorModule::post_update(FrameContext& frame)
{
    viewport.render(frame);
}

void EditorModule::end_frame(FrameContext& frame)
{
    im_gui_renderer.end_frame(frame);

    swapchain.present(frame);

    im_gui_renderer.render_subwindows();
}

void EditorModule::on_event(Event&)
{
}
} // portal
