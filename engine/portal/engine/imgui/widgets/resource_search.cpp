//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "resource_search.h"

#include "search_widget.h"

namespace portal::imgui
{
bool resource_search_popup(
    EditorContext& context,
    const std::string_view popup_id,
    ResourceType type,
    ResourceReference<Resource>& selected,
    bool* cleared,
    const std::string_view hint,
    const ImVec2 size
)
{
    return resource_search_popup(context, popup_id, selected, cleared, hint, size, {type});
}

bool resource_search_popup(
    EditorContext& context,
    std::string_view popup_id,
    ResourceReference<Resource>& selected,
    bool* cleared,
    std::string_view hint,
    ImVec2 size,
    std::initializer_list<ResourceType> types
)
{
    bool modified = false;
    ResourceReference<Resource> current = selected;

    auto& resource_registry = context.resource_registry;

    ImGui::SetNextWindowSize({size.x, 0.0f});
    static bool grap_focus = true;

    if (ImGui::BeginPopup(popup_id.data(), ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        static std::string search_string;

        if (ImGui::GetCurrentWindow()->Appearing)
        {
            grap_focus = true;
            search_string.clear();
        }

        // Search widget
        shift_cursor(3.f, 2.f);
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetCursorPosX() * 2.0f);
        search_widget(context, search_string, hint.data(), &grap_focus);

        const bool searching = !search_string.empty();

        // Clear property button
        if (cleared != nullptr)
        {
            ScopedStyle disable_border(ImGuiStyleVar_FrameBorderSize, 0.0f);
            ImGui::SetCursorPosX(0);
            ImGui::PushItemFlag(ImGuiItemFlags_NoNav, searching);
            if (ImGui::Button("CLEAR", {ImGui::GetWindowWidth(), 0.0f}))
            {
                *cleared = true;
                modified = true;
            }
            ImGui::PopItemFlag();
        }


        // List of resources
        {
            const auto list_id = ImGui::GetID("##search_list_box");
            if (ImGui::BeginListBox("##search_list_box", ImVec2(-FLT_MIN, 0.0f)))
            {
                bool forward_focus = false;
                const ImGuiContext& g = *GImGui;

                if (g.NavJustMovedToId != 0)
                {
                    if (g.NavJustMovedToId == list_id)
                    {
                        forward_focus = true;
                        // ActivateItemByID moves keyboard navigation focus inside of the window
                        ImGui::ActivateItemByID(list_id);
                        ImGui::SetKeyboardFocusHere(1);
                    }
                }

                std::vector<ResourceReference<Resource>> resources;

                for (auto& resource : resource_registry.list_all_resources())
                {
                    bool valid_type = false;
                    for (const auto type : types)
                    {
                        if (resource.get_resource_type() == type)
                        {
                            valid_type = true;
                            break;
                        }
                    }

                    if (!valid_type)
                        continue;

                    auto resource_name = resource.get_resource_name().string;

                    if (search_string.empty() || is_matching_search(resource_name, search_string))
                        resources.push_back(resource);
                }

                std::ranges::sort(resources, [](const auto& a, const auto& b) { return a.get_resource_name().string < b.get_resource_name().string; });

                for (auto& resource : resources)
                {
                    ImGui::PushID(resource.get_resource_id().string.data());
                    const bool is_selected = current == selected;
                    if (ImGui::Selectable(resource.get_resource_name().string.data(), is_selected))
                    {
                        current = selected;
                        selected = resource;
                        modified = true;
                    }
                    set_tooltip(resource.get_resource_id().string);

                    {
                        auto resource_type = to_string(resource.get_resource_type());
                        auto text_size = ImGui::CalcTextSize(resource_type.data());
                        auto rect_size = ImGui::GetItemRectSize();
                        float padding_x = ImGui::GetStyle().FramePadding.x;

                        ImGui::SameLine(rect_size.x - text_size.x - padding_x);

                        auto darker_text = context.theme.scoped_color(ImGuiCol_Text, imgui::ThemeColors::TextDarker);
                        ImGui::TextUnformatted(resource_type.data());
                    }

                    if (forward_focus)
                    {
                        forward_focus = false;
                    }
                    else if (is_selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }

                    ImGui::PopID();
                }

                ImGui::EndListBox();
            }
        }

        if (modified)
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    return modified;
}
}
