//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <imgui.h>
#include <imgui_internal.h>

#include <memory>


namespace portal
{
class Image;
}

namespace portal::ui
{
void shift_cursor_x(float distance);
void shit_cursor_y(float distance);
void shift_cursor(float x, float y);

ImRect get_item_rect();
ImRect rect_expand(const ImRect& rect, float x, float y);
ImRect rect_offset(const ImRect& rect, float x, float y);
ImRect rect_offset(const ImRect& rect, ImVec2 xy);

void draw_button_image(
    const std::shared_ptr<portal::Image>& image_normal,
    const std::shared_ptr<portal::Image>& image_hovered,
    const std::shared_ptr<portal::Image>& image_pressed,
    ImU32 tint_normal,
    ImU32 tint_hovered,
    ImU32 tint_pressed,
    ImVec2 rect_min,
    ImVec2 rect_max
    );

void draw_button_image(
    const std::shared_ptr<portal::Image>& image_normal,
    const std::shared_ptr<portal::Image>& image_hovered,
    const std::shared_ptr<portal::Image>& image_pressed,
    ImU32 tint_normal,
    ImU32 tint_hovered,
    ImU32 tint_pressed,
    ImRect rectangle
    );


void draw_button_image(
    const std::shared_ptr<portal::Image>& image,
    ImU32 tint_normal,
    ImU32 tint_hovered,
    ImU32 tint_pressed,
    ImVec2 rect_min,
    ImVec2 rect_max
    );

void draw_button_image(const std::shared_ptr<portal::Image>& image, ImU32 tint_normal, ImU32 tint_hovered, ImU32 tint_pressed, ImRect rect);

void draw_button_image(
    const std::shared_ptr<portal::Image>& image_normal,
    const std::shared_ptr<portal::Image>& image_hovered,
    const std::shared_ptr<portal::Image>& image_pressed,
    ImU32 tint_normal,
    ImU32 tint_hovered,
    ImU32 tint_pressed
    );

void draw_button_image(const std::shared_ptr<portal::Image>& image_normal, ImU32 tint_normal, ImU32 tint_hovered, ImU32 tint_pressed);

void render_window_outer_bounds(ImGuiWindow* window);

void update_window_manual_resize(ImGuiWindow* window, ImVec2& new_size, ImVec2& new_pos);

bool begin_menubar(const ImRect& bar_rectangle);
void end_menubar();

bool button_centered(const char* label, const ImVec2& size = ImVec2(0, 0));

void draw_border(ImRect rect, float thickness = 1.0f, float rounding = 0.0f, float offsetX = 0.0f, float offsetY = 0.0f);

} // namespace portal::ui
