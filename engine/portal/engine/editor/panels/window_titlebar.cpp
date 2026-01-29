//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "window_titlebar.h"

#include <imgui.h>

#include "portal/engine/renderer/vulkan/image/vulkan_texture.h"
#include "portal/engine/editor/editor_context.h"
#include "portal/engine/imgui/imgui_scoped.h"
#include "portal/engine/renderer/vulkan/image/vulkan_image.h"
#include "portal/third_party/imgui/backends/imgui_impl_vulkan.h"

namespace portal
{
WindowTitlebar::WindowTitlebar(ResourceRegistry& registry)
{
    // Helper to load a texture and create an ImGui descriptor
    auto load_icon = [&](StringId id) -> vk::DescriptorSet
    {
        auto texture = registry.immediate_load<renderer::vulkan::VulkanTexture>(id);
        auto vulkan_image = reference_cast<renderer::vulkan::VulkanImage>(texture->get_image());
        const auto& img_info = vulkan_image->get_image_info();
        return ImGui_ImplVulkan_AddTexture(
            img_info.sampler->get_vk_sampler(),
            img_info.view->get_vk_image_view(),
            static_cast<VkImageLayout>(vulkan_image->get_descriptor_image_info().imageLayout)
        );
    };

    // Load logo
    auto icon_image = registry.immediate_load<renderer::vulkan::VulkanTexture>(STRING_ID("engine/portal_icon_64x64"));
    icon_image->resize(22, 22, 1);
    auto vulkan_image = reference_cast<renderer::vulkan::VulkanImage>(icon_image->get_image());
    const auto& info = vulkan_image->get_image_info();
    logo_descriptor_set = ImGui_ImplVulkan_AddTexture(
        info.sampler->get_vk_sampler(),
        info.view->get_vk_image_view(),
        static_cast<VkImageLayout>(vulkan_image->get_descriptor_image_info().imageLayout)
    );

    // Load window button icons
    minimize_icon = load_icon(STRING_ID("engine/editor/icons/minimize"));
    maximize_icon = load_icon(STRING_ID("engine/editor/icons/maximize"));
    restore_icon = load_icon(STRING_ID("engine/editor/icons/restore"));
    close_icon = load_icon(STRING_ID("engine/editor/icons/close"));
}

WindowTitlebar::~WindowTitlebar()
{
    ImGui_ImplVulkan_RemoveTexture(logo_descriptor_set);
    ImGui_ImplVulkan_RemoveTexture(minimize_icon);
    ImGui_ImplVulkan_RemoveTexture(maximize_icon);
    ImGui_ImplVulkan_RemoveTexture(restore_icon);
    ImGui_ImplVulkan_RemoveTexture(close_icon);
}

void WindowTitlebar::on_gui_render(EditorContext& editor_context, FrameContext&)
{
    auto& window = editor_context.window;

    float y_offset = 0.0f;
    if (window.is_maximised())
    {
        y_offset = 6.0f;
    }

    {
        imgui::ScopedStyle window_padding(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 6.f));
        imgui::ScopedStyle frame_padding(ImGuiStyleVar_FramePadding, ImVec2(6.f, 10.f));

        height = ImGui::GetFrameHeight();

        auto viewport = ImGui::GetMainViewport();
        auto titlebar_position = ImVec2(viewport->Pos.x, viewport->Pos.y + y_offset);

        ImGui::SetNextWindowPos(titlebar_position);
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, height));
        ImGui::SetNextWindowViewport(viewport->ID);

        constexpr auto flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_MenuBar;

        imgui::ScopedWindow title_bar("##Titlebar", nullptr, flags);

        // screen space titlebar hitbox
        // ImVec2 p0 = ImGui::GetWindowPos();
        // ImVec2 p1 = ImVec2(p0.x + ImGui::GetWindowSize().x, p0.y + ImGui::GetWindowSize().y);
        // ImVec2 p0_screen = ImVec2(p0.x - viewport->Pos.x, p0.y - viewport->Pos.y);
        // ImVec2 p1_screen = ImVec2(p1.x - viewport->Pos.x, p1.y - viewport->Pos.y);

        const imgui::ScopedMenuBar menu_bar;
        if (menu_bar.is_open)
        {
            // ImGui::Image(
            //     reinterpret_cast<ImTextureID>(static_cast<VkDescriptorSet>(logo_descriptor_set)),
            //     ImVec2(22.f, 22.f),
            //     ImVec2(0, 0),
            //     ImVec2(1, 1),
            //     ImVec4(1, 1, 1, 1),
            //     ImVec4(0, 0, 0, 0)
            // );
            //
            // ImGui::SameLine(icon_width);


            auto item_label = [](std::string_view icon, std::string_view label)
            {
                if (icon.empty())
                {
                    return fmt::format("         {}", label);
                }
                return fmt::format("   {}  {}", icon, label);
            };

            // Menus (File, Edit, ...)
            {
                auto padding = imgui::ScopedStyle(ImGuiStyleVar_WindowPadding, ImVec2{3.f, 3.f});
                auto popup_background = editor_context.theme.scoped_color(ImGuiCol_PopupBg, imgui::ThemeColors::Background3);
                auto text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text2);

                const float original_border = ImGui::GetStyle().PopupBorderSize;
                ImGui::GetStyle().PopupBorderSize = 0.0f;

                {
                    ImGui::SetNextWindowSizeConstraints(ImVec2(150, 0), ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));
                    const imgui::ScopedMenu menu("File");
                    if (menu.is_open)
                    {
                        auto hovered = editor_context.theme.scoped_color(ImGuiCol_HeaderHovered, imgui::ThemeColors::Accent2);
                        auto menu_text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text1);
                        ImGui::MenuItem(item_label(ICON_FA_FILE_CIRCLE_PLUS, "New").c_str(), "Ctrl+N");
                        ImGui::MenuItem(item_label(ICON_FA_FOLDER_CLOSED, "Open...").c_str(), "Ctrl+O");
                        ImGui::Separator();
                        ImGui::MenuItem(item_label(ICON_FA_FLOPPY_DISK, "Save").c_str(), "Ctrl+S");
                        ImGui::Separator();
                        ImGui::MenuItem(item_label(ICON_FA_ARROW_UP_FROM_BRACKET, "Export...").c_str());
                        ImGui::MenuItem(item_label(ICON_FA_ARROW_UP_FROM_BRACKET, "Export...").c_str());
                        ImGui::MenuItem(item_label(ICON_FA_ARROW_UP_FROM_BRACKET, "Export...").c_str());
                        ImGui::Separator();
                        if (ImGui::MenuItem(item_label(ICON_FA_ROTATE_LEFT, "Undo").c_str(), "Ctrl+Z"))
                        {
                            editor_context.snapshot_manager.revert_snapshot();
                        }
                    }
                }

                {
                    ImGui::SetNextWindowSizeConstraints(ImVec2(150, 0), ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));
                    const imgui::ScopedMenu menu("View");
                    if (menu.is_open)
                    {
                        auto hovered = editor_context.theme.scoped_color(ImGuiCol_HeaderHovered, imgui::ThemeColors::Accent2);
                        auto menu_text_color = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::Text1);
                        if (ImGui::MenuItem(item_label(ICON_FA_ARROW_ROTATE_LEFT, "Restore To Default").c_str()))
                        {
                            editor_context.restore_default_settings();
                        }
                    }
                }

                ImGui::GetStyle().PopupBorderSize = original_border;
            }

        }

        // Window Buttons
        {
            // Position buttons at top-right of the window, flush with edges
            const ImVec2 window_pos = ImGui::GetWindowPos();
            const ImVec2 window_sz = ImGui::GetWindowSize();

            // Use actual window height for button sizing
            const auto button_size = ImVec2(46.0f, window_sz.y);
            const float total_width = button_size.x * 3.0f;
            const float buttons_y = window_pos.y;
            const float buttons_start_x = window_pos.x + window_sz.x - total_width;

            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            auto draw_window_button = [&](const int index, const vk::DescriptorSet icon_texture, const ImVec2 icon_sz, const bool is_close = false) -> bool
            {
                const auto pos = ImVec2(buttons_start_x + static_cast<float>(index) * button_size.x, buttons_y);
                const auto pos_max = ImVec2(pos.x + button_size.x, pos.y + button_size.y);

                // Set cursor for invisible button hit testing
                ImGui::SetCursorScreenPos(pos);
                ImGui::PushID(index);
                const bool clicked = ImGui::InvisibleButton("##btn", button_size);
                const bool hovered = ImGui::IsItemHovered();
                ImGui::PopID();

                // Draw background on hover
                if (hovered)
                {
                    ImU32 bg_color = is_close
                        ? ImGui::ColorConvertFloat4ToU32(editor_context.theme[imgui::ThemeColors::Error])
                        : ImGui::ColorConvertFloat4ToU32(editor_context.theme[imgui::ThemeColors::Primary1]);
                    draw_list->AddRectFilled(pos, pos_max, bg_color);
                }

                // Draw icon centered in button
                ImVec2 icon_pos = ImVec2(
                    std::floor(pos.x + (button_size.x - icon_sz.x) * 0.5f),
                    std::floor(pos.y + (button_size.y - icon_sz.y) * 0.5f)
                );
                ImVec2 icon_pos_max = ImVec2(icon_pos.x + icon_sz.x, icon_pos.y + icon_sz.y);

                draw_list->AddImage(
                    reinterpret_cast<ImTextureID>(static_cast<VkDescriptorSet>(icon_texture)),
                    icon_pos,
                    icon_pos_max
                );

                return clicked;
            };

            // Icon sizes matching the PNG dimensions
            constexpr auto minimize_size = ImVec2(10.0f, 1.0f); // Horizontal line
            constexpr auto square_size = ImVec2(10.0f, 10.0f);  // Square icons

            // Minimize
            if (draw_window_button(0, minimize_icon, minimize_size))
                window.minimize();

            // Maximize / Restore
            const vk::DescriptorSet max_icon_texture = window.is_maximised() ? restore_icon : maximize_icon;
            if (draw_window_button(1, max_icon_texture, square_size))
            {
                if (window.is_maximised())
                    window.restore();
                else
                    window.maximize();
            }

            // Close
            if (draw_window_button(2, close_icon, square_size, true))
                window.close();
        }
    }
}
} // portal
