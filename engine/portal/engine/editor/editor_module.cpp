//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//
#include "editor_module.h"

#include <imgui.h>
#include <ImGuizmo.h>
#include <imgui_internal.h>
#include "panel_manager.h"
#include "panels/details_panel.h"
#include "panels/content_browser/content_browser_panel.h"
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
    Window& window,
    entt::dispatcher& engine_dispatcher,
    entt::dispatcher& input_dispatcher
)
    : TaggedModule(stack, STRING_ID("Editor Module")),
      project(project),
      swapchain(swapchain),
      engine_dispatcher(engine_dispatcher),
      input_dispatcher(input_dispatcher),
      runtime_module(stack, project, context, swapchain, window),
      im_gui_renderer(get_dependency<ResourcesModule>().get_registry(), window, swapchain),
      icons(get_dependency<ResourcesModule>().get_registry()),
      editor_context(
          {},
          SnapshotManager{get_dependency<ResourcesModule>().get_registry()},
          window,
          engine_dispatcher,
          input_dispatcher,
          project,
          icons,
          get_dependency<ecs::Registry>(),
          get_dependency<ResourcesModule>().get_registry(),
          get_dependency<InputManager>()
      ),
      titlebar(editor_context),
      viewport(swapchain, runtime_module),
      input_router(get_dependency<SystemOrchestrator>(), engine_dispatcher, input_dispatcher)
{
    input_router.block_input();

    editor_context.restore_default_settings.connect<&EditorModule::restore_default_layout>(this);
    input_dispatcher.sink<KeyPressedEvent>().connect<&EditorModule::on_key_pressed>(this);
    input_dispatcher.sink<KeyReleasedEvent>().connect<&EditorModule::on_key_released>(this);

    setup_layout_config();
    panel_manager.add_panel<DetailsPanel>();
    panel_manager.add_panel<ContentBrowserPanel>(editor_context);
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

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || (ImGui::IsMouseClicked(ImGuiMouseButton_Right)))
    {
        ImGui::FocusWindow(GImGui->HoveredWindow);
    }

    io.ConfigWindowsResizeFromEdges = io.BackendFlags & ImGuiBackendFlags_HasMouseCursors;

    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoMove;
    const ImGuiViewport* imgui_viewport = ImGui::GetMainViewport();

    // Check if viewport size changed (external GLFW resize)
    const bool viewport_changed = (last_viewport_size.x != imgui_viewport->Size.x || last_viewport_size.y != imgui_viewport->Size.y);
    if (viewport_changed)
    {
        last_viewport_size = imgui_viewport->Size;
    }

    ImGui::SetNextWindowPos(ImVec2(imgui_viewport->Pos.x, imgui_viewport->Pos.y));
    // Only force window size if viewport changed externally
    if (viewport_changed)
    {
        ImGui::SetNextWindowSize(ImVec2(imgui_viewport->Size.x, imgui_viewport->Size.y));
    }
    ImGui::SetNextWindowViewport(imgui_viewport->ID);

    const auto& window_props = editor_context.window.get_properties();
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(static_cast<float>(window_props.minimum_extent.width), static_cast<float>(window_props.minimum_extent.height)),
        ImVec2(FLT_MAX, FLT_MAX)
    );

    const bool is_maximized = editor_context.window.is_maximised();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, is_maximized ? ImVec2(0.0f, 0.0f) : ImVec2(1.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, is_maximized ? 0.f : 3.0f);
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});
    ImGui::PushStyleColor(ImGuiCol_Border, editor_context.theme[imgui::ThemeColors::Background2]);

    ImGui::Begin("Main Window", nullptr, is_maximized ? window_flags | ImGuiWindowFlags_NoResize : window_flags);

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    // Check if the ImGui window was manually resized and update GLFW window accordingly
    const ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 window_size = window->Size;

    if (!viewport_changed && (window_size.x != imgui_viewport->Size.x || window_size.y != imgui_viewport->Size.y))
    {
        editor_context.window.resize(
            WindowExtent{
                static_cast<size_t>(window_size.x),
                static_cast<size_t>(window_size.y)
            }
        );
    }

    titlebar.on_gui_render(editor_context, frame);
    ImGui::SetCursorPosY(titlebar.get_height() + ImGui::GetCurrentWindow()->WindowPadding.y);

    const auto min_win_size = style.WindowMinSize;
    style.WindowMinSize.x = 325.0f;
    style.WindowMinSize.y = 150.0f;
    ImGui::DockSpace(ImGui::GetID("Editor"));
    style.WindowMinSize = min_win_size;


    panel_manager.on_gui_render(editor_context, frame);
    viewport.on_gui_update(editor_context, frame);

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

void EditorModule::on_key_pressed(const KeyPressedEvent& event)
{
    if (event.key == Key::RightMouseButton)
    {
        if (viewport.focused())
        {
            input_router.unblock_input();
            return;
        }
    }

    if (!input_router.is_input_blocked())
        return;

    if (event.key == Key::Z && event.modifiers & KeyModifierBits::Ctrl)
    {
        editor_context.snapshot_manager.undo();
        return;
    }

    if (event.key == Key::Y && event.modifiers & KeyModifierBits::Ctrl)
    {
        editor_context.snapshot_manager.redo();
        return;
    }

    if (event.key == Key::S && event.modifiers & KeyModifierBits::Ctrl)
    {
        if (event.modifiers & KeyModifierBits::Shift)
        {
            // TODO: IMPLEMENT SAVE AS
            return;
        }
        else
        {
            // TODO: save current scene
            // editor_context.resource_registry.save(scene_context->active_scene->get_id());
            return;
        }
    }

    if (event.key == Key::Q)
    {
        viewport.set_gizmo_type(-1);
    }
    if (event.key == Key::W)
    {
        viewport.set_gizmo_type(ImGuizmo::OPERATION::TRANSLATE);
    }
    if (event.key == Key::E)
    {
        viewport.set_gizmo_type(ImGuizmo::OPERATION::ROTATE);
    }
    if (event.key == Key::R)
    {
        viewport.set_gizmo_type(ImGuizmo::OPERATION::SCALE);
    }
}

void EditorModule::on_key_released(const KeyReleasedEvent& event)
{
    if (event.key == Key::RightMouseButton)
    {
        input_router.block_input();
    }

    if (input_router.is_input_blocked())
        return;
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
