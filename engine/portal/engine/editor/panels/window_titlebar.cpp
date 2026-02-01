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
ImGuiImages::ImGuiImages(ResourceRegistry& registry) : registry(registry) {}

ImGuiImages::~ImGuiImages()
{
    for (auto& [texture, descriptor] : images | std::views::values)
        ImGui_ImplVulkan_RemoveTexture(descriptor);
}

void ImGuiImages::load_image(const StringId& name, const StringId& texture_id)
{
    auto texture = registry.immediate_load<renderer::vulkan::VulkanTexture>(texture_id);
    const auto vulkan_image = reference_cast<renderer::vulkan::VulkanImage>(texture->get_image());
    const auto& img_info = vulkan_image->get_image_info();
    images[name] = {
        texture,
        ImGui_ImplVulkan_AddTexture(
            img_info.sampler->get_vk_sampler(),
            img_info.view->get_vk_image_view(),
            static_cast<VkImageLayout>(vulkan_image->get_descriptor_image_info().imageLayout)
        )
    };
}

vk::DescriptorSet ImGuiImages::get_descriptor(const StringId& name) const
{
    return images.at(name).descriptor;
}

ResourceReference<renderer::vulkan::VulkanTexture> ImGuiImages::get_texture(const StringId& name)
{
    return images.at(name).texture;
}

WindowTitlebar::WindowTitlebar(ResourceRegistry& registry, EditorContext& context) : icons(registry)
{
    // Load window button icons
    icons.load_image(STRING_ID("logo"), STRING_ID("engine/portal_icon_64x64"));

    // Window Icons
    icons.load_image(STRING_ID("minimize"), STRING_ID("engine/editor/icons/window/minimize"));
    icons.load_image(STRING_ID("maximize"), STRING_ID("engine/editor/icons/window/maximize"));
    icons.load_image(STRING_ID("restore"), STRING_ID("engine/editor/icons/window/restore"));
    icons.load_image(STRING_ID("close"), STRING_ID("engine/editor/icons/window/close"));

    // Files Menu Bar
    icons.load_image(STRING_ID("blocks"), STRING_ID("engine/editor/icons/generic/blocks"));
    icons.load_image(STRING_ID("boxes"), STRING_ID("engine/editor/icons/generic/boxes"));
    icons.load_image(STRING_ID("file-plus-corner"), STRING_ID("engine/editor/icons/generic/file-plus-corner"));
    icons.load_image(STRING_ID("folder-cog"), STRING_ID("engine/editor/icons/generic/folder-cog"));
    icons.load_image(STRING_ID("folder-open"), STRING_ID("engine/editor/icons/generic/folder-open"));
    icons.load_image(STRING_ID("folder-plus"), STRING_ID("engine/editor/icons/generic/folder-plus"));
    icons.load_image(STRING_ID("folder-clock"), STRING_ID("engine/editor/icons/generic/folder-clock"));
    icons.load_image(STRING_ID("folders"), STRING_ID("engine/editor/icons/generic/folders"));
    icons.load_image(STRING_ID("hammer"), STRING_ID("engine/editor/icons/generic/hammer"));
    icons.load_image(STRING_ID("import"), STRING_ID("engine/editor/icons/generic/import"));
    icons.load_image(STRING_ID("log-out"), STRING_ID("engine/editor/icons/generic/log-out"));
    icons.load_image(STRING_ID("save"), STRING_ID("engine/editor/icons/generic/save"));
    icons.load_image(STRING_ID("save-all"), STRING_ID("engine/editor/icons/generic/save-all"));

    // Edit Menu Bar
    icons.load_image(STRING_ID("cut"), STRING_ID("engine/editor/icons/generic/scissors"));
    icons.load_image(STRING_ID("duplicate"), STRING_ID("engine/editor/icons/generic/duplicate"));
    icons.load_image(STRING_ID("history"), STRING_ID("engine/editor/icons/generic/square-stack"));
    icons.load_image(STRING_ID("copy"), STRING_ID("engine/editor/icons/generic/copy"));
    icons.load_image(STRING_ID("undo"), STRING_ID("engine/editor/icons/generic/undo"));
    icons.load_image(STRING_ID("redo"), STRING_ID("engine/editor/icons/generic/redo"));
    icons.load_image(STRING_ID("paste"), STRING_ID("engine/editor/icons/generic/clipboard"));
    icons.load_image(STRING_ID("trash"), STRING_ID("engine/editor/icons/generic/trash"));

    active_color = target_color = context.theme[imgui::ThemeColors::AccentPrimaryLeft];
    previous_color = context.theme[imgui::ThemeColors::Background1];
}

void WindowTitlebar::on_gui_render(EditorContext& editor_context, FrameContext& frame_context)
{
    const ImVec2 window_padding = ImGui::GetCurrentWindow()->WindowPadding;

    constexpr auto buttons_offset = 0.f;
    const float titlebar_height = ImGui::GetFrameHeightWithSpacing() * 2;

    ImGui::SetCursorPos({window_padding.x, window_padding.y});
    const auto titlebar_min = ImGui::GetCursorScreenPos();
    const auto titlebar_max = ImVec2{
        ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - window_padding.y * 2.0f,
        ImGui::GetCursorScreenPos().y + titlebar_height
    };

    auto* draw_list = ImGui::GetWindowDrawList();
    auto titlebar_color = ImGui::GetColorU32(editor_context.theme[imgui::ThemeColors::Background1]);
    draw_list->AddRectFilled(titlebar_min, titlebar_max, titlebar_color);

    constexpr float animation_time = 0.15f;
    static float s_current_animation_timer = animation_time;

    if (animate_titlebar_color)
    {
        const float animation_percentage = 1.f - (s_current_animation_timer / animation_time);
        s_current_animation_timer -= frame_context.delta_time;

        const float r = std::lerp(previous_color.x, target_color.x, animation_percentage);
        const float g = std::lerp(previous_color.y, target_color.y, animation_percentage);
        const float b = std::lerp(previous_color.z, target_color.z, animation_percentage);

        active_color = ImVec4(r, g, b, 1.f);
        if (s_current_animation_timer <= 0.f)
        {
            s_current_animation_timer = animation_time;
            active_color = target_color;
            animate_titlebar_color = false;
        }
    }

    auto left_color = ImGui::GetColorU32(active_color);
    auto right_color = ImGui::GetColorU32(editor_context.theme[imgui::ThemeColors::AccentPrimaryRight]);

    draw_list->AddRectFilledMultiColor(
        titlebar_min,
        ImVec2(titlebar_min.x + 380.0f, titlebar_max.y),
        left_color,
        titlebar_color,
        titlebar_color,
        left_color
    );

    draw_list->AddRectFilledMultiColor(
        ImVec2(titlebar_max.x - 380.0f, titlebar_min.y),
        titlebar_max,
        titlebar_color,
        right_color,
        right_color,
        titlebar_color
    );

    // Logo
    {
        const auto logo_width = titlebar_height * 0.8f;
        const auto logo_height = titlebar_height * 0.8f;

        const ImVec2 logo_offset{2.f + window_padding.x, 2.f + window_padding.y};
        const ImVec2 logo_rect_start{titlebar_min.x + logo_offset.x, titlebar_min.y + logo_offset.y};
        const ImVec2 logo_rect_max{logo_rect_start.x + logo_width, logo_rect_start.y + logo_height};

        draw_list->AddImage(static_cast<VkDescriptorSet>(icons.get_descriptor(STRING_ID("logo"))), logo_rect_start, logo_rect_max);
    }

    ImGui::BeginHorizontal("Titlebar", {ImGui::GetWindowWidth() - window_padding.y * 2.0f, titlebar_height});
    static float move_offset_x;
    static float move_offset_y;
    const float w = ImGui::GetContentRegionAvail().x;

    constexpr auto button_spacing_1 = 17.f;
    constexpr auto button_spacing_2 = 15.f;
    constexpr auto button_spacing_3 = 18.f;

    constexpr float button_width = 14.f;
    constexpr float button_height = 14.f;
    constexpr float buttons_area_width = button_spacing_1 + button_spacing_2 + button_spacing_3 + button_width * 3.f;

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
        const float logo_offset = 16.0f * 2.0f + 41.0f + window_padding.x;
        ImGui::SetCursorPos(ImVec2(logo_offset, 4.0f));
        draw_menubar(editor_context);

        if (ImGui::IsItemHovered())
            titlebar_hovered = false;
    }

    const float menubar_right = ImGui::GetItemRectMax().x - ImGui::GetCurrentWindow()->Pos.x;


    // Centered Window title
    {
        auto current_position = ImGui::GetCursorPos();
        const char* title = "Portal Engine";
        auto scoped_font = imgui::ScopedFont(STRING_ID("BoldTitle"));

        ImVec2 text_size = ImGui::CalcTextSize(title);
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.5f - text_size.x * 0.5f, 2.0f + window_padding.y + 6.0f));

        ImGui::Text("%s [%s]", title, PORTAL_BUILD_CONFIG_NAME);
        ImGui::SetCursorPos(current_position);
    }

    // Current Scene Name
    {
        auto text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text1);
        auto* scene_context = std::any_cast<SceneContext>(&frame_context.scene_context);

        auto scene_name = scene_context->active_scene->get_id().string;
        ImGui::SetCursorPosX(menubar_right);
        imgui::shift_cursor(50.f, titlebar_height / 2.f);

        {
            auto scoped_font = imgui::ScopedFont(STRING_ID("Bold"));
            ImGui::Text("%s", scene_name.data());
        }
        imgui::set_tooltip(fmt::format("Current Scene ({})", scene_context->active_scene.get_resource_id()));

        constexpr float underline_thickness = 2.f;
        constexpr float underling_expand_width = 4.f;
        ImRect item_rect = imgui::expand_rect(imgui::get_item_rect(), underling_expand_width, 0.f);

        // Vertical Line
        item_rect.Max.x = item_rect.Min.x + underline_thickness;
        item_rect = imgui::rect_offset(item_rect, -underline_thickness * 2, 0.f);
        draw_list->AddRectFilled(
            item_rect.Min,
            item_rect.Max,
            ImGui::ColorConvertFloat4ToU32(editor_context.theme[imgui::ThemeColors::Primary1]),
            2.f
        );
    }

    // Project Name
    {
        auto text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text1);
        auto border = editor_context.theme.scoped_color(ImGuiCol_Border, imgui::ThemeColors::Primary2);

        const auto title = std::string(editor_context.project.get_name().string);
        const auto text_size = ImGui::CalcTextSize(title.c_str());
        const float right_offset = ImGui::GetWindowWidth() / 5.f;

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - right_offset - text_size.x);
        imgui::shift_cursor(0.f, 1.f + window_padding.y - titlebar_height / 2.f);

        {
            imgui::ScopedFont bold_font(STRING_ID("Bold"));
            ImGui::Text("%s", title.c_str());
        }
        imgui::set_tooltip(fmt::format("Current Project ({})", editor_context.project.get_project_directory().generic_string()));
        imgui::draw_border(imgui::expand_rect(imgui::get_item_rect(), 24.f, 68.f), 1.f, 3.f, {0.f, -60.f});
    }

    ImGui::ResumeLayout();

    // Window buttons
    const auto button_col_n = imgui::color_with_multiplied_value(editor_context.theme[imgui::ThemeColors::Text1], 0.9f);
    const auto button_col_h = imgui::color_with_multiplied_value(editor_context.theme[imgui::ThemeColors::Text1], 1.2f);
    const auto button_col_p = editor_context.theme[imgui::ThemeColors::Text2];

    ImGui::SetCursorPosY(titlebar_min.y + window_padding.y);
    // Minimize Button
    ImGui::Spring();
    imgui::shift_cursor(0.f, buttons_offset);
    {
        const auto icon_height = icons.get_texture(STRING_ID("minimize"))->get_height();
        const float pad_y = (button_height - static_cast<float>(icon_height)) / 2.f;
        if (ImGui::InvisibleButton("Minimize", ImVec2(button_width, button_height)))
        {
            editor_context.engine_dispatcher.enqueue<WindowRequestMinimizeEvent>();
        }

        imgui::draw_button_image(
            icons.get_descriptor(STRING_ID("minimize")),
            button_col_n,
            button_col_h,
            button_col_p,
            imgui::expand_rect(imgui::get_item_rect(), 0.f, -pad_y)
        );
    }

    // Maximize Button
    ImGui::Spring(-1.0f, button_spacing_1);
    imgui::shift_cursor(0.f, buttons_offset);
    {
        auto is_maximised = editor_context.window.is_maximised();
        if (ImGui::InvisibleButton("Maximize", ImVec2(button_width, button_height)))
        {
            editor_context.engine_dispatcher.enqueue<WindowRequestMaximizeOrRestoreEvent>();
        }

        imgui::draw_button_image(
            is_maximised ? icons.get_descriptor(STRING_ID("restore")) : icons.get_descriptor(STRING_ID("maximize")),
            button_col_n,
            button_col_h,
            button_col_p
        );
    }

    // Close Button
    {
        ImGui::Spring(-1.0f, button_spacing_2);
        imgui::shift_cursor(0.f, buttons_offset);

        if (ImGui::InvisibleButton("Close", ImVec2(button_width, button_height)))
        {
            editor_context.engine_dispatcher.enqueue<WindowRequestCloseEvent>();
        }

        imgui::draw_button_image(
            icons.get_descriptor(STRING_ID("close")),
            editor_context.theme[imgui::ThemeColors::Text1],
            imgui::color_with_multiplied_value(editor_context.theme[imgui::ThemeColors::Text1], 1.4f),
            button_col_p
        );
    }

    ImGui::Spring(-1.0f, button_spacing_3);
    ImGui::EndHorizontal();

    height = titlebar_height;

    //         // Menus (File, Edit, ...)
    //
    //             {
    //                 ImGui::SetNextWindowSizeConstraints(ImVec2(150, 0), ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));
    //                 const imgui::ScopedMenu menu("File");
    //                 if (menu.is_open)
    //                 {
    //                     ImGui::MenuItem(item_label(ICON_FA_FILE_CIRCLE_PLUS, "New").c_str(), "Ctrl+N");
    //                     ImGui::MenuItem(item_label(ICON_FA_FOLDER_CLOSED, "Open...").c_str(), "Ctrl+O");
    //                     ImGui::Separator();
    //                     ImGui::MenuItem(item_label(ICON_FA_FLOPPY_DISK, "Save").c_str(), "Ctrl+S");
    //                     ImGui::Separator();
    //                     ImGui::MenuItem(item_label(ICON_FA_ARROW_UP_FROM_BRACKET, "Export...").c_str());
    //                     ImGui::MenuItem(item_label(ICON_FA_ARROW_UP_FROM_BRACKET, "Export...").c_str());
    //                     ImGui::MenuItem(item_label(ICON_FA_ARROW_UP_FROM_BRACKET, "Export...").c_str());
    //                     ImGui::Separator();
    //                     if (ImGui::MenuItem(item_label(ICON_FA_ROTATE_LEFT, "Undo").c_str(), "Ctrl+Z"))
    //                     {
    //                         editor_context.snapshot_manager.revert_snapshot();
    //                     }
    //                 }
    //             }
    //
    //             {
    //                 ImGui::SetNextWindowSizeConstraints(ImVec2(150, 0), ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));
    //                 const imgui::ScopedMenu menu("View");
    //                 if (menu.is_open)
    //                 {
    //                     auto hovered = editor_context.theme.scoped_color(ImGuiCol_HeaderHovered, imgui::ThemeColors::Accent2);
    //                     auto menu_text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text1);
    //                     if (ImGui::MenuItem(item_label(ICON_FA_ARROW_ROTATE_LEFT, "Restore To Default").c_str()))
    //                     {
    //                         editor_context.restore_default_settings();
    //                     }
    //                 }
    //             }
    //
    //         }
}

void WindowTitlebar::draw_menubar(EditorContext& editor_context)
{
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

                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("folder-plus")), "Create Project...");
                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("folder-open")), "Open Project...");
                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("folder-clock")), "Open Recent");
                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("save-all")), "Save Project");
                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("file-plus-corner")), "New Scene");
                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("save")), "Save Scene", "Ctrl+S");
                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("import")), "Save Scene As...", "Ctrl+Shift+S");

                ImGui::Separator();
                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("hammer")), "Build All");

                if (imgui::begin_menu_with_image(icons.get_descriptor(STRING_ID("blocks")), "Build"))
                {
                    imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("folder-cog")), "Build Project Data");
                    imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("boxes")), "Build Shaders");
                    imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("folders")), "Build Resource DB");
                    ImGui::EndMenu();
                }

                ImGui::Separator();
                if (imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("log-out")), "Exit", "Alt + F4"))
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
                    icons.get_descriptor(STRING_ID("undo")),
                    "Undo",
                    "Ctrl+Z",
                    false,
                    editor_context.snapshot_manager.can_undo()
                ))
                    editor_context.snapshot_manager.undo();

                if (imgui::menu_item_with_image(
                    icons.get_descriptor(STRING_ID("redo")),
                    "Redo",
                    "Ctrl+Y",
                    false,
                    editor_context.snapshot_manager.can_redo()
                ))
                    editor_context.snapshot_manager.redo();

                if (imgui::begin_menu_with_image(icons.get_descriptor(STRING_ID("history")), "Snapshot History"))
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

                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("cut")), "Cut", "Ctrl+X");
                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("copy")), "Copy", "Ctrl+C");
                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("paste")), "Paste", "Ctrl+V");
                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("duplicate")), "Duplicate", "Ctrl+D");
                imgui::menu_item_with_image(icons.get_descriptor(STRING_ID("trash")), "Delete", "DELETE");
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
                ImGui::MenuItem("Statistics");
                ImGui::Separator();
                ImGui::MenuItem("Reset To Default");

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
                ImGui::MenuItem("Documentation");

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
