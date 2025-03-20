//
// Created by Jonatan Nevo on 15/03/2025.
//

#include "drawer.h"

namespace portal::gui
{
void Drawer::clear()
{
    dirty = false;
}

bool Drawer::is_dirty() const
{
    return dirty;
}

void Drawer::set_dirty(const bool dirty)
{
    this->dirty = dirty;
}

bool Drawer::header(const char* caption)
{
    return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
}

bool Drawer::checkbox(const char* caption, bool* value)
{
    const bool res = ImGui::Checkbox(caption, value);
    if (res)
        dirty = true;
    return res;
}

bool Drawer::checkbox(const char* caption, int32_t* value)
{
    bool val = (*value == 1);
    const bool res = ImGui::Checkbox(caption, &val);
    *value = val;
    if (res)
        dirty = true;
    return res;
}

bool Drawer::radio_button(const char* caption, int32_t* selected_option, const int32_t element_option)
{
    const bool res = ImGui::RadioButton(caption, selected_option, element_option);
    if (res)
        dirty = true;
    return res;
}

bool Drawer::input_float(const char* caption, float* value, const float step, const char* precision)
{
    const bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, precision);
    if (res)
        dirty = true;
    return res;
}

bool Drawer::slider_float(const char* caption, float* value, const float min, const float max)
{
    const bool res = ImGui::SliderFloat(caption, value, min, max);
    if (res)
        dirty = true;
    return res;
}

bool Drawer::slider_int(const char* caption, int32_t* value, const int32_t min, const int32_t max)
{
    const bool res = ImGui::SliderInt(caption, value, min, max);
    if (res)
        dirty = true;
    return res;
}

bool Drawer::combo_box(const char* caption, int32_t* item_index, const std::vector<std::string>& items)
{
    if (items.empty())
        return false;

    std::vector<const char*> char_items;
    char_items.reserve(items.size());
    for (const auto& item : items)
    {
        char_items.push_back(item.c_str());
    }

    const auto item_count = static_cast<int32_t>(char_items.size());
    const bool res = ImGui::Combo(caption, item_index, &char_items[0], item_count, item_count);
    if (res)
        dirty = true;
    return res;
}

bool Drawer::button(const char* caption)
{
    const bool res = ImGui::Button(caption);
    if (res)
        dirty = true;
    return res;
}

void Drawer::text(const char* format_str, ...)
{
    va_list args;
    va_start(args, format_str);
    ImGui::TextV(format_str, args);
    va_end(args);
}

template <>
bool Drawer::color_op_impl<Drawer::ColorOp::Edit, 3>(const char* caption, float* colors, const ImGuiColorEditFlags flags)
{
    return ImGui::ColorEdit3(caption, colors, flags);
}

template <>
bool Drawer::color_op_impl<Drawer::ColorOp::Edit, 4>(const char* caption, float* colors, const ImGuiColorEditFlags flags)
{
    return ImGui::ColorEdit4(caption, colors, flags);
}

template <>
bool Drawer::color_op_impl<Drawer::ColorOp::Pick, 3>(const char* caption, float* colors, const ImGuiColorEditFlags flags)
{
    return ImGui::ColorPicker3(caption, colors, flags);
}

template <>
bool Drawer::color_op_impl<Drawer::ColorOp::Pick, 4>(const char* caption, float* colors, const ImGuiColorEditFlags flags)
{
    return ImGui::ColorPicker4(caption, colors, flags);
}
} // portal
