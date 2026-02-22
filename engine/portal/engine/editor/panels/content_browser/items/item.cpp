//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "item.h"

#include <imgui.h>

#include "item_list.h"
#include "portal/core/files/file_system.h"
#include "portal/engine/editor/editor_context.h"
#include "portal/engine/editor/selection_system.h"
#include "portal/engine/imgui/imgui_scoped.h"
#include "portal/input/input_manager.h"

namespace portal::content_browser
{
static char s_rename_buffer[MAX_INPUT_BUFFER_LENGTH];

constexpr float thumbnail_size = 128.f;

Item::Item(
    const Type type,
    const StringId& resource_id,
    const std::string& name,
    const EditorIcon& icon
) : type(type),
    resource_id(resource_id),
    file_name(name),
    icon(icon)
{
    display_name = file_name;
}

void Item::on_render_begin() const
{
    ImGui::PushID(static_cast<int>(resource_id.id));
    ImGui::BeginGroup();
}

Action Item::on_render(
    const Entity selection_context,
    ItemList& item_list,
    EditorContext& editor_context
)
{
    Action result;

    set_display_name_from_file_name();

    constexpr float edge_offset = 4.f;
    const float text_line_height = ImGui::GetTextLineHeightWithSpacing() * 2.f + edge_offset * 2.f;
    const float info_panel_height = std::max(thumbnail_size * 0.5f, text_line_height);

    const auto top_left = ImGui::GetCursorScreenPos();
    const auto thump_bottom_right = ImVec2{top_left.x + thumbnail_size, top_left.y + thumbnail_size};
    const auto info_top_left = ImVec2{top_left.x, top_left.y + thumbnail_size};
    const auto bottom_right = ImVec2{top_left.x + thumbnail_size, top_left.y + thumbnail_size + info_panel_height};

    const bool is_focused = ImGui::IsWindowFocused();
    bool is_selected = SelectionSystem::is_selected(resource_id, selection_context);

    {
        imgui::ScopedStyle disable_item_spacing{ImGuiStyleVar_ItemSpacing, ImVec2{0.f, 0.f}};

        // Fill background
        //----------------
        if (type == Type::Directory)
        {
            auto* draw_list = ImGui::GetWindowDrawList();

            draw_list->AddRectFilled(top_left, thump_bottom_right, ImGui::GetColorU32(editor_context.theme[imgui::ThemeColors::Background2]));
            draw_list->AddRectFilled(
                info_top_left,
                bottom_right,
                ImGui::GetColorU32(editor_context.theme[imgui::ThemeColors::Background1]),
                6.0f,
                ImDrawFlags_RoundCornersBottom
            );
        }
        else if (ImGui::ItemHoverable(ImRect(top_left, bottom_right), ImGui::GetID(static_cast<int>(resource_id.id)), 0) || is_selected)
        {
            // If hovered or selected directory

            auto* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(
                info_top_left,
                bottom_right,
                ImGui::GetColorU32(editor_context.theme[imgui::ThemeColors::Background1]),
                6.0f
            );
        }


        // Thumbnail
        //==========

        // TODO: implement some thumbnail caching system
        ImGui::InvisibleButton("##thumbnailButton", ImVec2{thumbnail_size, thumbnail_size});
        // TODO: render thumbnail

        imgui::draw_button_image(
            editor_context.icons.get_descriptor(icon),
            IM_COL32(255, 255, 255, 225),
            IM_COL32(255, 255, 255, 255),
            IM_COL32(255, 255, 255, 255),
            imgui::expand_rect(imgui::get_item_rect(), -6.0f, -6.0f),
            ImVec2{0.f, 0.f},
            ImVec2{1.f, 1.f}
        );

        // Info Panel
        //-----------

        auto renaming_widget = [&]
        {
            ImGui::SetKeyboardFocusHere();
            ImGui::InputText("##rename", s_rename_buffer, MAX_INPUT_BUFFER_LENGTH);

            if (ImGui::IsItemDeactivatedAfterEdit() || ImGui::IsKeyPressed(ImGuiKey_Enter))
            {
                rename(s_rename_buffer);
                renaming = false;
                set_display_name_from_file_name();
                result |= ActionBit::Renamed;
            }
        };

        imgui::shift_cursor(edge_offset, edge_offset);

        if (type == Type::Directory)
        {
            ImGui::BeginVertical(
                fmt::format("InfoPanel{}", display_name).c_str(),
                ImVec2(thumbnail_size - edge_offset * 2.f, info_panel_height - edge_offset)
            );
            {
                ImGui::BeginHorizontal(file_name.c_str(), ImVec2(thumbnail_size - 2.0f, 0.0f));
                ImGui::Spring();
                {
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (thumbnail_size - edge_offset * 3.0f));
                    const float text_width = std::min(ImGui::CalcTextSize(display_name.c_str()).x, thumbnail_size);
                    if (renaming)
                    {
                        ImGui::SetNextItemWidth(thumbnail_size - edge_offset * 3.0f);
                        renaming_widget();
                    }
                    else
                    {
                        ImGui::SetNextItemWidth(text_width);
                        ImGui::TextUnformatted(display_name.c_str());
                    }
                    ImGui::PopTextWrapPos();
                }
                ImGui::Spring();
                ImGui::EndHorizontal();

                ImGui::Spring();
            }
            ImGui::EndVertical();
        }
        else
        {
            ImGui::BeginVertical(
                fmt::format("InfoPanel{}", display_name).c_str(),
                ImVec2(thumbnail_size - edge_offset * 2.f, info_panel_height - edge_offset)
            );
            {
                ImGui::BeginHorizontal("label", ImVec2(thumbnail_size - 2.0f, 0.0f));
                ImGui::Spring();
                {
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (thumbnail_size - edge_offset * 3.0f));
                    const float text_width = std::min(ImGui::CalcTextSize(display_name.c_str()).x, thumbnail_size);
                    if (renaming)
                    {
                        ImGui::SetNextItemWidth(thumbnail_size - edge_offset * 3.0f);
                        renaming_widget();
                    }
                    else
                    {
                        ImGui::SetNextItemWidth(text_width);
                        ImGui::TextUnformatted(display_name.c_str());
                    }
                    ImGui::PopTextWrapPos();
                }
                ImGui::Spring();
                ImGui::EndHorizontal();
            }
            ImGui::Spring();
            {
                ImGui::BeginHorizontal("resource_type", ImVec2(thumbnail_size - edge_offset * 2.f, 0.f));
                ImGui::Spring();
                {
                    auto metadata = editor_context.resource_registry.get_resource_database().find(resource_id);

                    if (metadata.has_value())
                    {
                        auto resource_metadata = metadata.value();
                        auto resource_type = to_string(resource_metadata.type);

                        auto darker_text = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::TextDarker);
                        imgui::ScopedFont small_font(STRING_ID("Small"));
                        ImGui::TextUnformatted(resource_type.data());
                    }
                    else
                    {
                        auto darker_text = editor_context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::TextDarker);
                        imgui::ScopedFont small_font(STRING_ID("Small"));
                        auto db_error = static_cast<DatabaseErrorBit>(metadata.error().get());
                        ImGui::Text("Invalid Metadata %s", portal::to_string(db_error).data());
                    }
                }
                ImGui::EndHorizontal();

                ImGui::Spring(-1.0f, edge_offset);
            }
            ImGui::EndVertical();
        }

        imgui::shift_cursor(-edge_offset, -edge_offset);

        if (!renaming)
        {
            if (ImGui::IsKeyDown(ImGuiKey_F2) && is_selected && is_focused)
                start_renaming();
        }
    }

    // End of the Item Group
    //======================
    ImGui::EndGroup();

    // Draw outline
    //-------------
    if (is_selected || ImGui::IsItemHovered())
    {
        auto item_rect = imgui::get_item_rect();
        auto* draw_list = ImGui::GetWindowDrawList();

        if (is_selected)
        {
            const bool mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsItemHovered();
            auto col_transition = imgui::color_with_multiplied_value(editor_context.theme[imgui::ThemeColors::Primary1], 0.8f);

            draw_list->AddRect(
                item_rect.Min,
                item_rect.Max,
                mouse_down ? static_cast<ImU32>(col_transition) : ImGui::GetColorU32(editor_context.theme[imgui::ThemeColors::Primary1]),
                6.f,
                type == Type::Directory ? 0 : ImDrawFlags_RoundCornersBottom,
                1.f
            );
        }
        else
        {
            if (type != Type::Directory)
            {
                draw_list->AddRect(
                    item_rect.Min,
                    item_rect.Max,
                    ImGui::GetColorU32(editor_context.theme[imgui::ThemeColors::Primary2]),
                    6.f,
                    ImDrawFlags_RoundCornersBottom,
                    1.f
                );
            }
        }
    }

    // Mouse Events handling
    //======================

    if (!is_selected)
        update_drop(item_list, result);

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        dragging = true;

        const auto& selections = SelectionSystem::get_selections(selection_context);
        if (!SelectionSystem::is_selected(resource_id, selection_context))
            result |= ActionBit::ClearSelections;


        if (!selections.empty())
        {
            for (auto& selected_item : selections)
            {
                // TODO: is this the correct id?
                size_t index = item_list.find_item(selected_item);
                if (index == ItemList::invalid_item)
                    continue;

                auto& item = item_list[index];
                ImGui::Image(static_cast<VkDescriptorSet>(editor_context.icons.get_descriptor(item->get_icon())), ImVec2(20, 20));
                ImGui::SameLine();
                const auto name = item->get_display_name();
                ImGui::TextUnformatted(name.c_str());
            }

            ImGui::SetDragDropPayload("resource_payload", selections.data(), sizeof(StringId) * selections.size());
        }

        result |= ActionBit::Selected;
        ImGui::EndDragDropSource();
    }

    if (ImGui::IsItemHovered())
    {
        result |= ActionBit::Hovered;

        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !is_renaming())
        {
            result |= ActionBit::Activated;
        }
        else
        {
            auto& input_manager = editor_context.input_manager;

            bool action = input_manager.is_key_pressed(Key::LeftMouseButton);
            is_selected = SelectionSystem::is_selected(resource_id, selection_context);
            bool skip_because_dragging = dragging && is_selected;

            if (action && !skip_because_dragging)
            {
                if (just_selected)
                    just_selected = false;

                if (is_selected && input_manager.is_key_pressed(Key::LeftControl) && !just_selected)
                {
                    result |= ActionBit::Deselected;
                }

                if (!is_selected)
                {
                    result |= ActionBit::Selected;
                    just_selected = true;
                }

                if (!input_manager.is_key_pressed(Key::LeftControl) && !input_manager.is_key_pressed(Key::LeftShift) && just_selected)
                {
                    result |= ActionBit::ClearSelections;
                }

                if (input_manager.is_key_pressed(Key::LeftShift))
                    result |= ActionBit::SelectToHere;
            }
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
    if (ImGui::BeginPopupContextItem("CBItemContextMenu"))
    {
         result |= ActionBit::Selected;
        on_context_menu_open(selection_context, result);
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    return result;
}

void Item::on_render_end() const
{
    ImGui::PopID();
    ImGui::NextColumn();
}

void Item::start_renaming()
{
    if (renaming)
        return;

    memset(s_rename_buffer, 0, MAX_INPUT_BUFFER_LENGTH);
    memcpy(s_rename_buffer, file_name.c_str(), file_name.size());
    renaming = true;
}

void Item::stop_renaming()
{
    renaming = false;
    set_display_name_from_file_name();
    memset(s_rename_buffer, 0, MAX_INPUT_BUFFER_LENGTH);
}

void Item::rename(const std::string& new_name)
{
    on_renamed(new_name);
}

void Item::set_display_name_from_file_name()
{
    // 0.00152587f is a magic number that is gained from graphing this equation in desmos and setting the y=25 at x=128
    constexpr int max_characters = static_cast<int>(0.00152587f * (thumbnail_size * thumbnail_size));

    if (file_name.size() > max_characters)
        display_name = file_name.substr(0, max_characters) + "...";
    else
        display_name = file_name;
}

void Item::on_context_menu_open(const Entity selection_context, Action& result)
{
    if (ImGui::MenuItem("Reload"))
        result |= ActionBit::Reload;

    if (SelectionSystem::selection_count(selection_context) == 1 && ImGui::MenuItem("Rename"))
        result |= ActionBit::StartRenaming;

    if (ImGui::MenuItem("Copy"))
        result |= ActionBit::Copy;

    if (ImGui::MenuItem("Duplicate"))
        result |= ActionBit::Duplicate;

    if (ImGui::MenuItem("Delete"))
        result |= ActionBit::OpenDeleteDialog;

    ImGui::Separator();

    if (ImGui::MenuItem("Show In Explorer"))
        result |= ActionBit::ShowInExplorer;

    if (ImGui::MenuItem("Open Externally"))
        result |= ActionBit::OpenExternal;

    render_custom_context_items();
}
} // portal
