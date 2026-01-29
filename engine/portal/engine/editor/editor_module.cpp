//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//
#include "editor_module.h"

#include <imgui.h>
#include <imgui_internal.h>
#include "panel_manager.h"
#include "panels/details_panel.h"
#include "portal/core/files/file_system.h"
#include "portal/engine/project/project.h"
#include "portal/engine/renderer/vulkan/vulkan_enum.h"
#include "portal/engine/renderer/vulkan/render_target/vulkan_render_target.h"
#include "portal/engine/scene/scene_context.h"
#include "portal/engine/window/glfw_window.h"
#include "portal/serialization/archive/json_archive.h"
#include "portal/third_party/imgui/ImGuiNotify.h"

namespace portal
{
constexpr static auto DEFAULT_CONFIGS_NAME = "default_editor_imgui.ini";
constexpr static auto CONFIGS_NAME = "editor_imgui.ini";

EditorModule::EditorModule(
    ModuleStack& stack,
    Project& project,
    renderer::vulkan::VulkanContext& context,
    renderer::vulkan::VulkanSwapchain& swapchain,
    Window& window
)
    : TaggedModule(stack, STRING_ID("Editor Module")),
      project(project),
      swapchain(swapchain),
      runtime_module(stack, project, context, swapchain, window),
      im_gui_renderer(get_dependency<ResourcesModule>().get_registry(), window, swapchain),
      editor_context(
          {},
          SnapshotManager{get_dependency<ResourcesModule>().get_registry()},
          window
      ),
      titlebar(get_dependency<ResourcesModule>().get_registry()),
      viewport(swapchain, runtime_module)
{
    setup_layout_config();
    editor_context.restore_default_settings.connect<&EditorModule::restore_default_layout>(this);
    panel_manager.add_panel<DetailsPanel>();
}

void EditorModule::begin_frame(FrameContext& frame)
{
    frame.rendering_context = swapchain.prepare_frame(frame);
    frame.scene_context = SceneContext{get_dependency<SystemOrchestrator>().get_active_scene()};
    editor_context.snapshot_manager.set_scene_id(get_dependency<SystemOrchestrator>().get_active_scene().get_resource_id());

    auto render_target = swapchain.get_current_render_target(false);
    im_gui_renderer.begin_frame(frame, render_target);
}

void EditorModule::gui_update(FrameContext& frame)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    io.ConfigWindowsResizeFromEdges = io.BackendFlags & ImGuiBackendFlags_HasMouseCursors;

    titlebar.on_gui_render(editor_context, frame);

    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    const ImGuiViewport* imgui_viewport = ImGui::GetMainViewport();

    const float titlebar_height = titlebar.get_height();
    ImGui::SetNextWindowPos(ImVec2(imgui_viewport->Pos.x, imgui_viewport->Pos.y + titlebar_height));
    ImGui::SetNextWindowSize(ImVec2(imgui_viewport->Size.x, imgui_viewport->Size.y - titlebar_height));
    ImGui::SetNextWindowViewport(imgui_viewport->ID);

#ifdef PORTAL_PLATFORM_WINDOWS
    // TODO: this can be a generic function on the window class
    const auto& vulkan_window = dynamic_cast<const GlfwWindow&>(editor_context.window);
    bool is_maximized = glfwGetWindowAttrib(vulkan_window.get_handle(), GLFW_MAXIMIZED) == GLFW_TRUE;
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

    {
        imgui::ScopedStyle disable_round_windows(ImGuiStyleVar_WindowRounding, 0.f);
        imgui::ScopedStyle disable_borders(ImGuiStyleVar_WindowBorderSize, 0.f);

        auto background_color = editor_context.theme.scoped_color(ImGuiCol_WindowBg, imgui::ThemeColors::Background3);
        auto button_color = editor_context.theme.scoped_color(ImGuiCol_Button, imgui::ThemeColors::Background3);

        ImGui::RenderNotifications();
    }
}

void EditorModule::post_update(FrameContext& frame)
{
    viewport.render(frame);
}

void EditorModule::end_frame(FrameContext& frame)
{
    im_gui_renderer.end_frame(frame);

    if (!editor_context.window.is_minimized())
        swapchain.present(frame);

    im_gui_renderer.render_subwindows();
}

void EditorModule::on_event(Event&)
{
}

void EditorModule::setup_layout_config()
{
    auto editor_config_path = project.get_config_directory() / CONFIGS_NAME;
    if (!FileSystem::exists(editor_config_path))
    {
        FileSystem::copy(project.get_engine_config_directory() / DEFAULT_CONFIGS_NAME, editor_config_path);
    }
    auto& io = ImGui::GetIO();
    config_path_storage = editor_config_path.string();
    io.IniFilename = config_path_storage.c_str();
}

void EditorModule::restore_default_layout()
{
    auto editor_config_path = project.get_config_directory() / CONFIGS_NAME;
    auto default_config_path = project.get_engine_config_directory() / DEFAULT_CONFIGS_NAME;

    if (FileSystem::exists(default_config_path))
    {
        FileSystem::copy(default_config_path, editor_config_path);

        ImGui::GetIO().IniFilename = nullptr;
        config_path_storage = editor_config_path.string();
        ImGui::LoadIniSettingsFromDisk(config_path_storage.c_str());
        ImGui::GetIO().IniFilename = config_path_storage.c_str();
    }
}
} // portal
