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
WindowTitlebar::WindowTitlebar(ResourceRegistry& registry, EditorContext& context)
{
    // Helper to load a texture and create an ImGui descriptor
    auto load_icon = [&](StringId id) -> IconData
    {
        auto texture = registry.immediate_load<renderer::vulkan::VulkanTexture>(id);
        const auto vulkan_image = reference_cast<renderer::vulkan::VulkanImage>(texture->get_image());
        const auto& img_info = vulkan_image->get_image_info();
        return {
            texture,
            ImGui_ImplVulkan_AddTexture(
                img_info.sampler->get_vk_sampler(),
                img_info.view->get_vk_image_view(),
                static_cast<VkImageLayout>(vulkan_image->get_descriptor_image_info().imageLayout)
            )
        };
    };

    // Load window button icons
    logo_icon = load_icon(STRING_ID("engine/portal_icon_64x64"));
    minimize_icon = load_icon(STRING_ID("engine/editor/icons/minimize"));
    maximize_icon = load_icon(STRING_ID("engine/editor/icons/maximize"));
    restore_icon = load_icon(STRING_ID("engine/editor/icons/restore"));
    close_icon = load_icon(STRING_ID("engine/editor/icons/close"));

    active_color = target_color = context.theme[imgui::ThemeColors::AccentPrimaryLeft];
    previous_color = context.theme[imgui::ThemeColors::Background1];
}

WindowTitlebar::~WindowTitlebar()
{
    ImGui_ImplVulkan_RemoveTexture(logo_icon.descriptor);
    ImGui_ImplVulkan_RemoveTexture(minimize_icon.descriptor);
    ImGui_ImplVulkan_RemoveTexture(maximize_icon.descriptor);
    ImGui_ImplVulkan_RemoveTexture(restore_icon.descriptor);
    ImGui_ImplVulkan_RemoveTexture(close_icon.descriptor);
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

        draw_list->AddImage(static_cast<VkDescriptorSet>(logo_icon.descriptor), logo_rect_start, logo_rect_max);
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
        const auto icon_height = minimize_icon.texture->get_height();
        const float pad_y = (button_height - static_cast<float>(icon_height)) / 2.f;
        if (ImGui::InvisibleButton("Minimize", ImVec2(button_width, button_height)))
        {
            editor_context.engine_dispatcher.enqueue<WindowRequestMinimizeEvent>();
        }

        imgui::draw_button_image(
            minimize_icon.descriptor,
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

        imgui::draw_button_image(is_maximised ? restore_icon.descriptor : maximize_icon.descriptor, button_col_n, button_col_h, button_col_p);
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
            close_icon.descriptor,
            editor_context.theme[imgui::ThemeColors::Text1],
            imgui::color_with_multiplied_value(editor_context.theme[imgui::ThemeColors::Text1], 1.4f),
            button_col_p
        );
    }

    ImGui::Spring(-1.0f, button_spacing_3);
    ImGui::EndHorizontal();

    height = titlebar_height;

    // float y_offset = 0.0f;
    // if (window.is_maximised())
    // {
    //     y_offset = 6.0f;
    // }
    //
    // {
    //     imgui::ScopedStyle window_padding(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 6.f));
    //     imgui::ScopedStyle frame_padding(ImGuiStyleVar_FramePadding, ImVec2(6.f, 10.f));
    //
    //     height = ImGui::GetFrameHeight();
    //
    //     auto viewport = ImGui::GetMainViewport();
    //     auto titlebar_position = ImVec2(viewport->Pos.x, viewport->Pos.y + y_offset);
    //
    //     ImGui::SetNextWindowPos(titlebar_position);
    //     ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, height));
    //     ImGui::SetNextWindowViewport(viewport->ID);
    //
    //     constexpr auto flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
    //         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
    //         ImGuiWindowFlags_MenuBar;
    //
    //     imgui::ScopedWindow title_bar("##Titlebar", nullptr, flags);
    //
    //     // screen space titlebar hitbox
    //     // ImVec2 p0 = ImGui::GetWindowPos();
    //     // ImVec2 p1 = ImVec2(p0.x + ImGui::GetWindowSize().x, p0.y + ImGui::GetWindowSize().y);
    //     // ImVec2 p0_screen = ImVec2(p0.x - viewport->Pos.x, p0.y - viewport->Pos.y);
    //     // ImVec2 p1_screen = ImVec2(p1.x - viewport->Pos.x, p1.y - viewport->Pos.y);
    //
    //     const imgui::ScopedMenuBar menu_bar;
    //     if (menu_bar.is_open)
    //     {
    //         // Calculate icon size based on menu bar height
    //         const float menu_bar_height = ImGui::GetFrameHeight();
    //         const float icon_size = menu_bar_height * 0.8f; // 80% of menu bar height
    //
    //         // Calculate vertical offset to center the icon
    //         const float icon_offset_y = (menu_bar_height - icon_size) * 0.5f;
    //
    //         // Save the original Y position for menu items
    //         const float original_y = ImGui::GetCursorPosY();
    //
    //         // Offset icon vertically to center it
    //         ImGui::SetCursorPosY(original_y + icon_offset_y);
    //         ImGui::Image(
    //             reinterpret_cast<ImTextureID>(static_cast<VkDescriptorSet>(logo_icon)),
    //             ImVec2(icon_size, icon_size),
    //             ImVec2(0, 0),
    //             ImVec2(1, 1),
    //             ImVec4(1, 1, 1, 1),
    //             ImVec4(0, 0, 0, 0)
    //         );
    //
    //         // Continue on same line and restore original Y position for menus
    //         ImGui::SameLine(icon_size + 5.f);
    //         ImGui::SetCursorPosY(original_y);
    //
    //         auto item_label = [](std::string_view icon, std::string_view label)
    //         {
    //             if (icon.empty())
    //             {
    //                 return fmt::format("         {}", label);
    //             }
    //             return fmt::format("   {}  {}", icon, label);
    //         };
    //
    //         // Menus (File, Edit, ...)
    //         {
    //             auto padding = imgui::ScopedStyle(ImGuiStyleVar_WindowPadding, ImVec2{3.f, 3.f});
    //             auto popup_background = editor_context.theme.scoped_color(ImGuiCol_PopupBg, imgui::ThemeColors::Background3);
    //             auto text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text2);
    //
    //             const float original_border = ImGui::GetStyle().PopupBorderSize;
    //             ImGui::GetStyle().PopupBorderSize = 0.0f;
    //
    //             {
    //                 ImGui::SetNextWindowSizeConstraints(ImVec2(150, 0), ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));
    //                 const imgui::ScopedMenu menu("File");
    //                 if (menu.is_open)
    //                 {
    //                     auto hovered = editor_context.theme.scoped_color(ImGuiCol_HeaderHovered, imgui::ThemeColors::Accent2);
    //                     auto menu_text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text1);
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
    //             ImGui::GetStyle().PopupBorderSize = original_border;
    //         }
    //
    //     }
    //
    //     // Window Buttons
    //     {
    //         // Position buttons at top-right of the window, flush with edges
    //         const ImVec2 window_pos = ImGui::GetWindowPos();
    //         const ImVec2 window_sz = ImGui::GetWindowSize();
    //
    //         // Use actual window height for button sizing
    //         const auto button_size = ImVec2(46.0f, window_sz.y);
    //         const float total_width = button_size.x * 3.0f;
    //         const float buttons_y = window_pos.y;
    //         const float buttons_start_x = window_pos.x + window_sz.x - total_width;
    //
    //         ImDrawList* draw_list = ImGui::GetWindowDrawList();
    //
    //         auto draw_window_button = [&](const int index, const vk::DescriptorSet icon_texture, const ImVec2 icon_sz, const bool is_close = false) -> bool
    //         {
    //             const auto pos = ImVec2(buttons_start_x + static_cast<float>(index) * button_size.x, buttons_y);
    //             const auto pos_max = ImVec2(pos.x + button_size.x, pos.y + button_size.y);
    //
    //             // Set cursor for invisible button hit testing
    //             ImGui::SetCursorScreenPos(pos);
    //             ImGui::PushID(index);
    //             const bool clicked = ImGui::InvisibleButton("##btn", button_size);
    //             const bool hovered = ImGui::IsItemHovered();
    //             ImGui::PopID();
    //
    //             // Draw background on hover
    //             if (hovered)
    //             {
    //                 ImU32 bg_color = is_close
    //                     ? ImGui::ColorConvertFloat4ToU32(editor_context.theme[imgui::ThemeColors::Error])
    //                     : ImGui::ColorConvertFloat4ToU32(editor_context.theme[imgui::ThemeColors::Primary1]);
    //                 draw_list->AddRectFilled(pos, pos_max, bg_color);
    //             }
    //
    //             // Draw icon centered in button
    //             ImVec2 icon_pos = ImVec2(
    //                 std::floor(pos.x + (button_size.x - icon_sz.x) * 0.5f),
    //                 std::floor(pos.y + (button_size.y - icon_sz.y) * 0.5f)
    //             );
    //             ImVec2 icon_pos_max = ImVec2(icon_pos.x + icon_sz.x, icon_pos.y + icon_sz.y);
    //
    //             draw_list->AddImage(
    //                 reinterpret_cast<ImTextureID>(static_cast<VkDescriptorSet>(icon_texture)),
    //                 icon_pos,
    //                 icon_pos_max
    //             );
    //
    //             return clicked;
    //         };
    //
    //         // Icon sizes matching the PNG dimensions
    //         constexpr auto minimize_size = ImVec2(10.0f, 1.0f); // Horizontal line
    //         constexpr auto square_size = ImVec2(10.0f, 10.0f);  // Square icons
    //
    //         // Minimize
    //         if (draw_window_button(0, minimize_icon, minimize_size))
    //             window.minimize();
    //
    //         // Maximize / Restore
    //         const vk::DescriptorSet max_icon_texture = window.is_maximised() ? restore_icon : maximize_icon;
    //         if (draw_window_button(1, max_icon_texture, square_size))
    //         {
    //             if (window.is_maximised())
    //                 window.restore();
    //             else
    //                 window.maximize();
    //         }
    //
    //         // Close
    //         if (draw_window_button(2, close_icon, square_size, true))
    //             window.close();
}

void WindowTitlebar::draw_menubar(EditorContext& editor_context)
{
    const ImRect menubar_rect = {
        ImGui::GetCursorPos(),
        {ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x, ImGui::GetFrameHeightWithSpacing()}
    };

    imgui::ScopedRectangleMenuBar menubar(menubar_rect);
    if (menubar)
    {
        auto push_dark_text_if_active = [&editor_context](const char* name) -> std::optional<imgui::ScopedColor>
        {
            if (ImGui::IsPopupOpen(name))
            {
                return std::make_optional<imgui::ScopedColor>(ImGuiCol_Text, editor_context.theme[imgui::ThemeColors::Text2]);
            }
            return std::nullopt;
        };

        {
            [[maybe_unused]] auto pushed_color = push_dark_text_if_active("File");

            if (ImGui::BeginMenu("File"))
            {
                {
                    auto header_hovered = editor_context.theme.scoped_color(ImGuiCol_HeaderHovered, imgui::ThemeColors::Background4);

                    ImGui::MenuItem("Create Project...");
                    ImGui::MenuItem("Open Project...");
                    ImGui::MenuItem("Open Recent");
                    ImGui::MenuItem("Save Project");
                    ImGui::MenuItem("New Scene");
                    ImGui::MenuItem("Save Scene", "Ctrl+S");
                    ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S");

                    ImGui::Separator();
                    ImGui::MenuItem("Build All");
                    if (ImGui::BeginMenu("Build"))
                    {
                        ImGui::MenuItem("Build Project Data");
                        ImGui::MenuItem("Build Shaders");
                        ImGui::MenuItem("Build Resource DB");
                        ImGui::EndMenu();
                    }

                    ImGui::Separator();
                    if (ImGui::MenuItem("Exit", "Alt + F4"))
                    {
                        editor_context.engine_dispatcher.enqueue<WindowRequestCloseEvent>();
                    }
                }

                ImGui::EndMenu();
            }
        }
        {
            [[maybe_unused]] auto pushed_color = push_dark_text_if_active("Edit");

            if (ImGui::BeginMenu("Edit"))
            {
                {
                    auto header_hovered = editor_context.theme.scoped_color(ImGuiCol_HeaderHovered, imgui::ThemeColors::Background4);


                    if (ImGui::MenuItem("Undo", "Ctrl+Z", false, editor_context.snapshot_manager.can_undo()))
                        editor_context.snapshot_manager.undo();

                    if (ImGui::MenuItem("Redo", "Ctrl+Y", false, editor_context.snapshot_manager.can_redo()))
                        editor_context.snapshot_manager.redo();

                    if (ImGui::BeginMenu("Snapshot History"))
                    {
                        auto current_snapshot = editor_context.snapshot_manager.get_current_snapshot_index();
                        for (auto [index, title, timestamp]: editor_context.snapshot_manager.list_snapshots())
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

                    ImGui::MenuItem("Cut", "Ctrl+X");
                    ImGui::MenuItem("Copy", "Ctrl+C");
                    ImGui::MenuItem("Paste", "Ctrl+V");
                    ImGui::MenuItem("Duplicate", "Ctrl+D");
                    ImGui::MenuItem("Delete", "DELETE");
                }

                ImGui::EndMenu();
            }
        }
        {
            [[maybe_unused]] auto pushed_color = push_dark_text_if_active("View");

            if (ImGui::BeginMenu("View"))
            {
                {
                    auto header_hovered = editor_context.theme.scoped_color(ImGuiCol_HeaderHovered, imgui::ThemeColors::Background4);

                    ImGui::MenuItem("Viewports");
                    ImGui::MenuItem("Statistics");
                    ImGui::Separator();
                    ImGui::MenuItem("Reset To Default");
                }

                ImGui::EndMenu();
            }
        }
        {
            [[maybe_unused]] auto pushed_color = push_dark_text_if_active("Tools");

            if (ImGui::BeginMenu("Tools"))
            {
                ImGui::EndMenu();
            }
        }
        {
            [[maybe_unused]] auto pushed_color = push_dark_text_if_active("Help");

            if (ImGui::BeginMenu("Help"))
            {
                {
                    auto header_hovered = editor_context.theme.scoped_color(ImGuiCol_HeaderHovered, imgui::ThemeColors::Background4);
                    ImGui::MenuItem("About");
                    ImGui::MenuItem("Documentation");
                }

                ImGui::EndMenu();
            }
        }
    }
}
} // portal
