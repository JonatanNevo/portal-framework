//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include <vulkan/vulkan.hpp>

#include "imgui_fonts.h"
#include "theme/serializers.h"
#include "portal/core/strings/string_id.h"

namespace portal::imgui
{
//=========================================================================================

/**
 * Dynamically reflects a `consts` struct and draws it in a debug window for fine controls of spacing in real time.
 *
 * @tparam T The consts struct type
 * @param label A label for the debug window, to allow multiple windows in parallel
 * @param consts and instance of the consts struct
 */
template <typename T>
void draw_consts_controls(const char* label, T& consts)
{
    ImGui::Begin(label);

    constexpr auto N = glz::reflect<T>::size;
    if constexpr (N > 0)
    {
        glz::for_each<N>(
            [&]<size_t I>()
            {
                auto& field = glz::get_member(consts, glz::get<I>(glz::to_tie(consts)));
                auto& name = glz::reflect<T>::keys[I];

                if constexpr (std::is_same_v<std::remove_cvref_t<decltype(field)>, float>)
                {
                    ImGui::DragFloat(name.data(), &field, 0.01f);
                }
                if constexpr (std::is_same_v<std::remove_cvref_t<decltype(field)>, ImVec2>)
                {
                    float* float_ptr = &field.x;
                    ImGui::DragFloat2(name.data(), float_ptr, 0.01f);
                }
                if constexpr (std::is_same_v<std::remove_cvref_t<decltype(field)>, int>)
                {
                    ImGui::DragInt(name.data(), &field);
                }
            }
        );
    }

    ImGui::End();
}

//=========================================================================================
typedef int OutlineFlags;

enum OutlineFlags_
{
    OutlineFlags_None              = 0,      // draw no activity outline
    OutlineFlags_WhenHovered       = 1 << 1, // draw an outline when item is hovered
    OutlineFlags_WhenActive        = 1 << 2, // draw an outline when item is active
    OutlineFlags_WhenInactive      = 1 << 3, // draw an outline when item is inactive
    OutlineFlags_HighlightActive   = 1 << 4, // when active, the outline is in highlight colour
    OutlineFlags_NoHighlightActive = OutlineFlags_WhenHovered | OutlineFlags_WhenActive | OutlineFlags_WhenInactive,
    OutlineFlags_NoOutlineInactive = OutlineFlags_WhenHovered | OutlineFlags_WhenActive | OutlineFlags_HighlightActive,
    OutlineFlags_All               = OutlineFlags_WhenHovered | OutlineFlags_WhenActive | OutlineFlags_WhenInactive | OutlineFlags_HighlightActive,
};

void draw_item_activity_outline(
    OutlineFlags flags = OutlineFlags_All,
    ImColor color_highlight = IM_COL32(236, 158, 36, 255),
    float rounding = GImGui->Style.FrameRounding
);

//=========================================================================================
// Rect Operations
ImRect get_item_rect();
ImRect expand_rect(const ImRect& rect, float size);
ImRect expand_rect(const ImRect& rect, float x, float y);
ImRect rect_offset(const ImRect& rect, ImVec2 offset);
ImRect rect_offset(const ImRect& rect, float x, float y);

//=========================================================================================
bool is_item_disabled();
bool is_item_hovered(float delay_in_seconds = 0.1f, ImGuiHoveredFlags flags = ImGuiHoveredFlags_None);

//=========================================================================================
void set_tooltip(std::string_view tooltip, float delay_in_seconds = 0.1f, bool allow_when_disabled = true, ImVec2 padding = ImVec2{5.f, 5.f});
void shift_cursor(float x, float y);
void shift_cursor(ImVec2 vec);

//=========================================================================================
void draw_border(ImRect rect, float thickness = 1.f, float rounding = 0.f, ImVec2 offset = {0.f, 0.f});

//=========================================================================================
ImColor color_with_multiplied_value(const ImColor& color, float multiplier);

//=========================================================================================
void draw_button_image(
    vk::DescriptorSet image,
    ImColor tint_normal,
    ImColor tint_hovered,
    ImColor tint_pressed,
    ImVec2 uv0 = {0.f, 0.f},
    ImVec2 uv1 = {1.f, 1.f}
);

void draw_button_image(
    vk::DescriptorSet image,
    ImColor tint_normal,
    ImColor tint_hovered,
    ImColor tint_pressed,
    ImRect rect,
    ImVec2 uv0 = {0.f, 0.f},
    ImVec2 uv1 = {1.f, 1.f}
);

void draw_button_image(
    vk::DescriptorSet image,
    ImColor tint_normal,
    ImColor tint_hovered,
    ImColor tint_pressed,
    ImVec2 rect_min,
    ImVec2 rect_max,
    ImVec2 uv0 = {0.f, 0.f},
    ImVec2 uv1 = {1.f, 1.f}
);

void draw_button_image(
    vk::DescriptorSet image_normal,
    vk::DescriptorSet image_hovered,
    vk::DescriptorSet image_pressed,
    ImColor tint_normal,
    ImColor tint_hovered,
    ImColor tint_pressed,
    ImVec2 rect_min,
    ImVec2 rect_max,
    ImVec2 uv0 = {0.f, 0.f},
    ImVec2 uv1 = {1.f, 1.f}
);
}
