//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "window_titlebar.h"

#include <imgui.h>
#include <fmt/chrono.h>

#include "portal/engine/renderer/vulkan/image/vulkan_texture.h"
#include "portal/engine/editor/editor_context.h"
#include "portal/engine/imgui/imgui_scoped.h"
#include "portal/engine/project/project.h"
#include "portal/engine/renderer/vulkan/image/vulkan_image.h"
#include "portal/engine/scene/scene_context.h"
#include "portal/engine/window/window_events.h"
#include "portal/third_party/imgui/backends/imgui_impl_vulkan.h"

namespace portal
{

WindowTitlebar::WindowTitlebar(EditorContext& context)
{
    active_color = target_color = context.theme[imgui::ThemeColors::AccentPrimaryLeft];
    previous_color = context.theme[imgui::ThemeColors::Background1];
}

struct WindowTitleConsts
{
    float buttons_offset = 0.f;
    float titlebar_height_scale = 1.2f;
    float color_background_width = 380.f;

    float logo_scale = 0.8f;
    float logo_offset = 3.f;

    float menubar_x_offset_component = 9.f;
    float menubar_y_offset = 4.f;

    float window_title_y_offset = 6.f;

    float button_spacing_1 = 17.f;
    float button_spacing_2 = 15.f;
    float button_spacing_3 = 18.f;

    float button_normal_multiplier = 0.9f;
    float button_hovered_multiplier = 1.2f;

    float button_width = 14.f;
    float button_height = 14.f;

    float animation_time = 0.15f;
};

void WindowTitlebar::on_gui_render(EditorContext& editor_context, FrameContext& frame_context)
{
    constexpr WindowTitleConsts consts;

    const ImVec2 window_padding = ImGui::GetCurrentWindow()->WindowPadding;

    const float titlebar_height = ImGui::GetFrameHeightWithSpacing() * consts.titlebar_height_scale;

    ImGui::SetCursorPos({window_padding.x, window_padding.y});
    const auto titlebar_min = ImGui::GetCursorScreenPos();
    const auto titlebar_max = ImVec2{
        ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - window_padding.y * 2.0f,
        ImGui::GetCursorScreenPos().y + titlebar_height
    };

    auto* draw_list = ImGui::GetWindowDrawList();
    auto titlebar_color = ImGui::GetColorU32(editor_context.theme[imgui::ThemeColors::Background1]);
    draw_list->AddRectFilled(titlebar_min, titlebar_max, titlebar_color);

    static float s_current_animation_timer = consts.animation_time;

    if (animate_titlebar_color)
    {
        const float animation_percentage = 1.f - (s_current_animation_timer / consts.animation_time);
        s_current_animation_timer -= frame_context.delta_time;

        const float r = std::lerp(previous_color.x, target_color.x, animation_percentage);
        const float g = std::lerp(previous_color.y, target_color.y, animation_percentage);
        const float b = std::lerp(previous_color.z, target_color.z, animation_percentage);

        active_color = ImVec4(r, g, b, 1.f);
        if (s_current_animation_timer <= 0.f)
        {
            s_current_animation_timer = consts.animation_time;
            active_color = target_color;
            animate_titlebar_color = false;
        }
    }

    auto left_color = ImGui::GetColorU32(active_color);
    auto right_color = ImGui::GetColorU32(editor_context.theme[imgui::ThemeColors::AccentPrimaryRight]);

    draw_list->AddRectFilledMultiColor(
        titlebar_min,
        ImVec2(titlebar_min.x + consts.color_background_width, titlebar_max.y),
        left_color,
        titlebar_color,
        titlebar_color,
        left_color
    );

    draw_list->AddRectFilledMultiColor(
        ImVec2(titlebar_max.x - consts.color_background_width, titlebar_min.y),
        titlebar_max,
        titlebar_color,
        right_color,
        right_color,
        titlebar_color
    );

    const auto logo_width = titlebar_height * consts.logo_scale;
    const auto logo_height = titlebar_height * consts.logo_scale;

    // Logo
    {
        constexpr ImVec2 logo_offset{consts.logo_offset, consts.logo_offset};
        const ImVec2 logo_rect_start{titlebar_min.x + logo_offset.x, titlebar_min.y + logo_offset.y};
        const ImVec2 logo_rect_max{logo_rect_start.x + logo_width, logo_rect_start.y + logo_height};

        draw_list->AddImage(static_cast<VkDescriptorSet>(editor_context.icons.get_descriptor(EditorIcon::Logo)), logo_rect_start, logo_rect_max);
    }

    ImGui::BeginHorizontal("Titlebar", {ImGui::GetWindowWidth() - window_padding.y * 2.0f, titlebar_height});
    static float move_offset_x;
    static float move_offset_y;
    const float w = ImGui::GetContentRegionAvail().x;

    constexpr float buttons_area_width = consts.button_spacing_1 + consts.button_spacing_2 + consts.button_spacing_3 + consts.button_width * 3.f;

    // Title bar drag area
    const auto* root_window = ImGui::GetCurrentWindow();
    const float window_width = root_window->RootWindow->Size.x;
    ImGui::SetNextItemAllowOverlap();
    if (ImGui::InvisibleButton("##titleBarDragZone", ImVec2(w - buttons_area_width, titlebar_height), ImGuiButtonFlags_PressedOnClick))
    {
        const auto point = ImGui::GetMousePos();
        const auto rect = root_window->Rect();
        move_offset_x = point.x - rect.Min.x;
        move_offset_y = point.y - rect.Min.y;
    }

    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
    {
        editor_context.engine_dispatcher.enqueue<WindowRequestMaximizeOrRestoreEvent>();
    }
    else if (ImGui::IsItemActive())
    {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            const auto point = ImGui::GetMousePos();
            editor_context.engine_dispatcher.enqueue<WindowDragEvent>(
                window_width,
                glm::vec2{point.x, point.y},
                glm::vec2{move_offset_x, move_offset_y}
            );
        }
    }

    // Draw Menubar
    ImGui::SuspendLayout();
    {
        const float logo_offset = consts.menubar_x_offset_component + logo_width + window_padding.x;
        ImGui::SetCursorPos(ImVec2(logo_offset, consts.menubar_y_offset));
        draw_menubar(editor_context, frame_context);

        if (ImGui::IsItemHovered())
            titlebar_hovered = false;
    }

    // Centered Window title
    {
        auto current_position = ImGui::GetCursorPos();
        const char* title = "Portal Engine";

        {
            auto scoped_font = imgui::ScopedFont(STRING_ID("BoldTitle"));
            ImVec2 text_size = ImGui::CalcTextSize(title);
            ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.5f - text_size.x * 0.5f, window_padding.y + consts.window_title_y_offset));

            ImGui::Text("%s [%s]", title, PORTAL_BUILD_CONFIG_NAME);
        }

        imgui::set_tooltip(fmt::format("Current Project ({})", editor_context.project.get_project_directory().generic_string()));
        ImGui::SetCursorPos(current_position);
    }

    ImGui::ResumeLayout();

    // Window buttons
    const auto button_col_n = imgui::color_with_multiplied_value(editor_context.theme[imgui::ThemeColors::Text1], consts.button_normal_multiplier);
    const auto button_col_h = imgui::color_with_multiplied_value(editor_context.theme[imgui::ThemeColors::Text1], consts.button_hovered_multiplier);
    const auto button_col_p = editor_context.theme[imgui::ThemeColors::Text2];

    ImGui::SetCursorPosY(titlebar_min.y + window_padding.y);
    // Minimize Button
    ImGui::Spring();
    imgui::shift_cursor(0.f, consts.buttons_offset);
    {
        const auto icon_height = editor_context.icons.get_texture(EditorIcon::Minimize)->get_height();
        const float pad_y = (consts.button_height - static_cast<float>(icon_height)) / 2.f;
        if (ImGui::InvisibleButton("Minimize", ImVec2(consts.button_width, consts.button_height)))
        {
            editor_context.engine_dispatcher.enqueue<WindowRequestMinimizeEvent>();
        }

        imgui::draw_button_image(
            editor_context.icons.get_descriptor(EditorIcon::Minimize),
            button_col_n,
            button_col_h,
            button_col_p,
            imgui::expand_rect(imgui::get_item_rect(), 0.f, -pad_y)
        );
    }

    // Maximize Button
    ImGui::Spring(-1.0f, consts.button_spacing_1);
    imgui::shift_cursor(0.f, consts.buttons_offset);
    {
        auto is_maximised = editor_context.window.is_maximised();
        if (ImGui::InvisibleButton("Maximize", ImVec2(consts.button_width, consts.button_height)))
        {
            editor_context.engine_dispatcher.enqueue<WindowRequestMaximizeOrRestoreEvent>();
        }

        imgui::draw_button_image(
            is_maximised ? editor_context.icons.get_descriptor(EditorIcon::Restore) : editor_context.icons.get_descriptor(EditorIcon::Maximize),
            button_col_n,
            button_col_h,
            button_col_p
        );
    }

    // Close Button
    {
        ImGui::Spring(-1.0f, consts.button_spacing_2);
        imgui::shift_cursor(0.f, consts.buttons_offset);

        if (ImGui::InvisibleButton("Close", ImVec2(consts.button_width, consts.button_height)))
        {
            editor_context.engine_dispatcher.enqueue<WindowRequestCloseEvent>();
        }

        imgui::draw_button_image(
            editor_context.icons.get_descriptor(EditorIcon::Close),
            editor_context.theme[imgui::ThemeColors::Text1],
            imgui::color_with_multiplied_value(editor_context.theme[imgui::ThemeColors::Text1], 1.4f),
            button_col_p
        );
    }

    ImGui::Spring(-1.0f, consts.button_spacing_3);
    ImGui::EndHorizontal();

    height = titlebar_height;
}

void WindowTitlebar::draw_menubar(EditorContext& editor_context, FrameContext& frame)
{
    auto* scene_context = std::any_cast<SceneContext>(&frame.scene_context);
    auto& icons = editor_context.icons;
    const ImRect menubar_rect = {
        ImGui::GetCursorPos(),
        {ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x, ImGui::GetFrameHeightWithSpacing()}
    };

    imgui::ScopedGroup menubar_group;
    imgui::ScopedRectangleMenuBar menubar(menubar_rect);
    if (menubar)
    {
        auto padding = imgui::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2{3.f, 3.f});
        auto rounding = imgui::ScopedStyle(ImGuiStyleVar_PopupRounding, 2.f);
        auto border_size = imgui::ScopedStyle(ImGuiStyleVar_PopupBorderSize, 1.f);

        auto popup_background = editor_context.theme.scoped_color(ImGuiCol_PopupBg, imgui::ThemeColors::Background3);

        bool menu_open = ImGui::IsPopupOpen("##menubar", ImGuiPopupFlags_AnyPopupId);

        if (menu_open)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, editor_context.theme[imgui::ThemeColors::AccentPrimaryLeft]);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, editor_context.theme[imgui::ThemeColors::AccentPrimaryLeft]);
        }

        auto pop_item_highlight = [&menu_open]
        {
            if (menu_open)
            {
                ImGui::PopStyleColor(3);
                menu_open = false;
            }
        };

        auto push_dark_text_if_active = [&editor_context](const char* name)
        {
            if (ImGui::IsPopupOpen(name))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, editor_context.theme[imgui::ThemeColors::Text2]);
                return true;
            }
            return false;
        };

        {
            bool color_pushed = push_dark_text_if_active("File");
            imgui::ScopedMenu menu("File");
            if (menu)
            {
                pop_item_highlight();
                color_pushed = false;

                auto hovered = editor_context.theme.scoped_color(ImGuiCol_HeaderHovered, imgui::ThemeColors::Accent2);
                auto menu_text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text1);

                imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::NewProject), "Create Project...");
                imgui::set_tooltip("Not Implemented!");
                imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::NewProject), "Open Project...");
                imgui::set_tooltip("Not Implemented!");
                imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::OpenRecent), "Open Recent");
                imgui::set_tooltip("Not Implemented!");
                imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::SaveAll), "Save Project");
                imgui::set_tooltip("Not Implemented!");
                imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::NewScene), "New Scene");
                imgui::set_tooltip("Not Implemented!");
                if (imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::Save), "Save Scene", "Ctrl+S"))
                    editor_context.resource_registry.save(scene_context->active_scene->get_id());

                imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::SaveAs), "Save Scene As...", "Ctrl+Shift+S");
                imgui::set_tooltip("Not Implemented!");

                ImGui::Separator();
                imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::Build), "Build All");
                imgui::set_tooltip("Not Implemented!");

                if (imgui::begin_menu_with_image(icons.get_descriptor(EditorIcon::BuildMenu), "Build"))
                {
                    imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::BuildProject), "Build Project Data");
                    imgui::set_tooltip("Not Implemented!");
                    imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::BuildShaders), "Build Shaders");
                    imgui::set_tooltip("Not Implemented!");
                    imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::BuildResourceDB), "Build Resource DB");
                    imgui::set_tooltip("Not Implemented!");
                    ImGui::EndMenu();
                }

                ImGui::Separator();
                if (imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::Exit), "Exit", "Alt + F4"))
                {
                    editor_context.engine_dispatcher.enqueue<WindowRequestCloseEvent>();
                }
            }

            if (color_pushed)
            {
                ImGui::PopStyleColor();
            }
        }
        {
            bool color_pushed = push_dark_text_if_active("Edit");
            imgui::ScopedMenu menu("Edit");
            if (menu)
            {
                pop_item_highlight();
                color_pushed = false;

                auto hovered = editor_context.theme.scoped_color(ImGuiCol_HeaderHovered, imgui::ThemeColors::Accent2);
                auto menu_text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text1);

                if (imgui::menu_item_with_image(
                    icons.get_descriptor(EditorIcon::Undo),
                    "Undo",
                    "Ctrl+Z",
                    false,
                    editor_context.snapshot_manager.can_undo()
                ))
                    editor_context.snapshot_manager.undo();

                if (imgui::menu_item_with_image(
                    icons.get_descriptor(EditorIcon::Redo),
                    "Redo",
                    "Ctrl+Y",
                    false,
                    editor_context.snapshot_manager.can_redo()
                ))
                    editor_context.snapshot_manager.redo();

                if (imgui::begin_menu_with_image(icons.get_descriptor(EditorIcon::History), "Snapshot History"))
                {
                    auto current_snapshot = editor_context.snapshot_manager.get_current_snapshot_index();
                    for (auto [index, title, timestamp] : editor_context.snapshot_manager.list_snapshots())
                    {
                        if (index == current_snapshot)
                            ImGuiFonts::push_font(STRING_ID("Bold"));

                        auto menu_item_title = fmt::format("{}###{}", title.string.data(), index);
                        auto date = fmt::format("{:%Y-%m-%d %H:%M:%S}", timestamp);
                        if (ImGui::MenuItem(menu_item_title.c_str(), date.c_str()))
                        {
                            editor_context.snapshot_manager.revert_snapshot(index);
                        }

                        if (index == current_snapshot)
                            ImGuiFonts::pop_font();
                    }
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::Cut), "Cut", "Ctrl+X");
                imgui::set_tooltip("Not Implemented!");
                imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::Copy), "Copy", "Ctrl+C");
                imgui::set_tooltip("Not Implemented!");
                imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::Paste), "Paste", "Ctrl+V");
                imgui::set_tooltip("Not Implemented!");
                imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::Duplicate), "Duplicate", "Ctrl+D");
                imgui::set_tooltip("Not Implemented!");
                imgui::menu_item_with_image(icons.get_descriptor(EditorIcon::Delete), "Delete", "DELETE");
                imgui::set_tooltip("Not Implemented!");
            }

            if (color_pushed)
            {
                ImGui::PopStyleColor();
            }
        }

        {
            bool color_pushed = push_dark_text_if_active("View");

            if (ImGui::BeginMenu("View"))
            {
                pop_item_highlight();
                color_pushed = false;
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, editor_context.theme[imgui::ThemeColors::Background4]);

                ImGui::MenuItem("Viewports");
                imgui::set_tooltip("Not Implemented!");
                ImGui::MenuItem("Statistics");
                imgui::set_tooltip("Not Implemented!");
                ImGui::Separator();
                ImGui::MenuItem("Reset To Default");
                imgui::set_tooltip("Not Implemented!");

                ImGui::PopStyleColor();
                ImGui::EndMenu();
            }

            if (color_pushed)
            {
                ImGui::PopStyleColor();
            }
        }

        {
            bool color_pushed = push_dark_text_if_active("Tools");

            if (ImGui::BeginMenu("Tools"))
            {
                pop_item_highlight();
                color_pushed = false;
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, editor_context.theme[imgui::ThemeColors::Background4]);

                ImGui::MenuItem("Something");
                imgui::set_tooltip("Not Implemented!");

                ImGui::PopStyleColor();
                ImGui::EndMenu();
            }

            if (color_pushed)
            {
                ImGui::PopStyleColor();
            }
        }

        {
            bool color_pushed = push_dark_text_if_active("Help");

            if (ImGui::BeginMenu("Help"))
            {
                pop_item_highlight();
                color_pushed = false;
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, editor_context.theme[imgui::ThemeColors::Background4]);

                ImGui::MenuItem("About");
                imgui::set_tooltip("Not Implemented!");
                ImGui::MenuItem("Documentation");
                imgui::set_tooltip("Not Implemented!");

                ImGui::PopStyleColor();
                ImGui::EndMenu();
            }

            if (color_pushed)
            {
                ImGui::PopStyleColor();
            }
        }

        if (menu_open)
            ImGui::PopStyleColor(2);
    }
}
} // portal
