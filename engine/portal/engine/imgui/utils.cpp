//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#define IMGUI_DEFINE_MATH_OPERATORS

#include "utils.h"
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>

#include "imgui_scoped.h"
#include "widgets/edit_vec3.h"

namespace portal::imgui
{
void draw_item_activity_outline(OutlineFlags flags, ImColor color_highlight, float rounding)
{
    if (is_item_disabled())
        return;

    auto* draw_list = ImGui::GetWindowDrawList();
    const auto rect = expand_rect(get_item_rect(), 1.f);
    if (flags & OutlineFlags_WhenActive && ImGui::IsItemActive())
    {
        if (flags & OutlineFlags_HighlightActive)
        {
            draw_list->AddRect(rect.Min, rect.Max, color_highlight, rounding, 0, 1.5f);
        }
        else
        {
            draw_list->AddRect(rect.Min, rect.Max, ImColor(60, 60, 60), rounding, 0, 1.5f);
        }
    }
    else if (flags & OutlineFlags_WhenHovered && ImGui::IsItemHovered() && !ImGui::IsItemActive())
    {
        draw_list->AddRect(rect.Min, rect.Max, ImColor(60, 60, 60), rounding, 0, 1.5f);
    }
    else if (flags & OutlineFlags_WhenInactive && !ImGui::IsItemHovered() && !ImGui::IsItemActive())
    {
        draw_list->AddRect(rect.Min, rect.Max, ImColor(50, 50, 50), rounding, 0, 1.0f);
    }
}

ImRect get_item_rect()
{
    return ImRect{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
}

ImRect expand_rect(const ImRect& rect, const float size)
{
    return expand_rect(rect, size, size);
}

ImRect expand_rect(const ImRect& rect, const float x, const float y)
{
    auto result = rect;
    result.Min.x -= x;
    result.Min.y -= y;
    result.Max.x += x;
    result.Max.y += y;
    return result;
}

ImRect rect_offset(const ImRect& rect, const ImVec2 offset)
{
    return rect_offset(rect, offset.x, offset.y);
}

ImRect rect_offset(const ImRect& rect, const float x, const float y)
{
    ImRect result = rect;
    result.Min.x += x;
    result.Min.y += y;
    result.Max.x += x;
    result.Max.y += y;
    return result;
}

bool is_item_disabled()
{
    return ImGui::GetItemFlags() & ImGuiItemFlags_Disabled;
}

bool is_item_hovered(const float delay_in_seconds, const ImGuiHoveredFlags flags)
{
    return ImGui::IsItemHovered(flags) && GImGui->HoveredIdTimer > delay_in_seconds;
}

void set_tooltip(std::string_view tooltip, float delay_in_seconds, bool allow_when_disabled, ImVec2 padding)
{
    if (is_item_hovered(delay_in_seconds, allow_when_disabled ? ImGuiHoveredFlags_AllowWhenDisabled : ImGuiHoveredFlags_None))
    {
        ScopedStyle tooltip_padding(ImGuiStyleVar_WindowPadding, padding);
        ImGui::SetTooltip("%s", tooltip.data());
    }
}

void shift_cursor(const float x, const float y)
{
    const ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(cursor.x + x, cursor.y + y));
}

void shift_cursor(const ImVec2 vec)
{
    shift_cursor(vec.x, vec.y);
}

void draw_border(const ImRect rect, const float thickness, const float rounding, const ImVec2 offset)
{
    auto min = rect.Min;
    min.x -= thickness;
    min.y -= thickness;
    min.x += offset.x;
    min.y += offset.y;

    auto max = rect.Max;
    max.x += thickness;
    max.y += thickness;
    max.x += offset.x;
    max.y += offset.y;

    auto* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(min, max, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Border)), rounding, 0, thickness);
}

ImColor color_with_multiplied_value(const ImColor& color, const float multiplier)
{
    const ImVec4& raw_value = color.Value;
    float hue, sat, val;
    ImGui::ColorConvertRGBtoHSV(raw_value.x, raw_value.y, raw_value.z, hue, sat, val);
    return ImColor::HSV(hue, sat, std::min(val * multiplier, 1.0f));
}

ImColor color_with_multiplied_saturation(const ImColor& color, const float multiplier)
{
    const ImVec4& raw_value = color.Value;
    float hue, sat, val;
    ImGui::ColorConvertRGBtoHSV(raw_value.x, raw_value.y, raw_value.z, hue, sat, val);
    return ImColor::HSV(hue, std::min(sat * multiplier, 1.0f), val);
}

void draw_button_image(
    const vk::DescriptorSet image,
    const ImColor tint_normal,
    const ImColor tint_hovered,
    const ImColor tint_pressed,
    [[maybe_unused]] const ImVec2 size,
    const ImVec2 uv0,
    const ImVec2 uv1
)
{
    ImVec2 rect_min, rect_max;
    if (size.x != 0.f && size.y != 0.f)
    {
        const ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
        rect_min = bb.Min;
        rect_max = bb.Max;
    }
    else
    {
        rect_min = ImGui::GetItemRectMin();
        rect_max = ImGui::GetItemRectMax();
    }

    return draw_button_image(
        image,
        image,
        image,
        tint_normal,
        tint_hovered,
        tint_pressed,
        rect_min,
        rect_max,
        uv0,
        uv1
    );
}

void draw_button_image(
    const vk::DescriptorSet image,
    const ImColor tint_normal,
    const ImColor tint_hovered,
    const ImColor tint_pressed,
    const ImRect rect,
    const ImVec2 uv0,
    const ImVec2 uv1
)
{
    return draw_button_image(image, image, image, tint_normal, tint_hovered, tint_pressed, rect.Min, rect.Max, uv0, uv1);
}

void draw_button_image(
    const vk::DescriptorSet image,
    const ImColor tint_normal,
    const ImColor tint_hovered,
    const ImColor tint_pressed,
    const ImVec2 rect_min,
    const ImVec2 rect_max,
    const ImVec2 uv0,
    const ImVec2 uv1
)
{
    return draw_button_image(image, image, image, tint_normal, tint_hovered, tint_pressed, rect_min, rect_max, uv0, uv1);
}

void draw_button_image(
    const vk::DescriptorSet image_normal,
    const vk::DescriptorSet image_hovered,
    const vk::DescriptorSet image_pressed,
    const ImColor tint_normal,
    const ImColor tint_hovered,
    const ImColor tint_pressed,
    const ImVec2 rect_min,
    const ImVec2 rect_max,
    const ImVec2 uv0,
    const ImVec2 uv1
)
{
    auto* draw_list = ImGui::GetWindowDrawList();
    if (ImGui::IsItemActive())
        draw_list->AddImage(static_cast<VkDescriptorSet>(image_pressed), rect_min, rect_max, uv0, uv1, tint_pressed);
    else if (ImGui::IsItemHovered())
        draw_list->AddImage(static_cast<VkDescriptorSet>(image_hovered), rect_min, rect_max, uv0, uv1, tint_hovered);
    else
        draw_list->AddImage(static_cast<VkDescriptorSet>(image_normal), rect_min, rect_max, uv0, uv1, tint_normal);
}

static int s_ui_context_id = 0;
static uint32_t s_counter = 0;
static std::array<char, 16 + 3> s_id_buffer = {'#', '#', '\0'};
static std::array<char, 1024 + 1> s_label_id_buffer = {};

const char* generate_id()
{
    std::snprintf(s_id_buffer.data() + 2, 16, "%u", s_counter++);
    return s_id_buffer.data();
}

const char* generate_label_id(std::string_view label)
{
    *fmt::format_to_n(s_label_id_buffer.data(), s_label_id_buffer.size(), "{}##{}", label, s_counter++).out = 0;
    return s_label_id_buffer.data();
}

void push_id()
{
    ImGui::PushID(s_ui_context_id++);
    s_counter = 0;
}

void pop_id()
{
    ImGui::PopID();
    s_ui_context_id--;
}

void help_marker(const std::string_view description)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(description.data());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void begin_property_grid(const size_t columns)
{
    push_id();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
    ImGui::Columns(static_cast<int>(columns));
}

void end_property_grid()
{
    ImGui::Columns(1);
    underline();
    ImGui::PopStyleVar(2);
    // shift_cursor(0.0f, 18.0f);
    pop_id();
}

void begin_property(const std::string_view label, const std::string_view help_text)
{
    // shift_cursor(10.f, 9.f);
    ImGui::TextUnformatted(label.data());

    if (!help_text.empty())
    {
        ImGui::SameLine();
        help_marker(help_text);
    }

    ImGui::NextColumn();
    // shift_cursor(0, 4.f);
    ImGui::PushItemWidth(-1.f);
}

void end_property()
{
    ImGui::PopItemWidth();
    ImGui::NextColumn();
    // underline();
}

bool property_button(const std::string_view label, const std::string_view button_text, const std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::Button(fmt::format("{}##{}", button_text.data(), label.data()).c_str());

    end_property();
    return modified;
}

bool property_radio(
    std::string_view label,
    int& choice,
    const std::map<int, const std::string_view>& options,
    std::string_view help_text,
    const std::map<int, const std::string_view>& options_help_text
)
{
    bool modified = false;
    begin_property(label, help_text);

    for (auto [value, option] : options)
    {
        auto radio_label = fmt::format("{}##{}", option, label);
        if (ImGui::RadioButton(radio_label.c_str(), &choice, value))
            modified = true;

        if (auto option_help_text = options_help_text.find(value); option_help_text != options_help_text.end())
        {
            if (!option_help_text->second.empty())
            {
                ImGui::SameLine();
                help_marker(option_help_text->second);
            }
        }
    }

    end_property();
    return modified;
}

bool property(std::string_view label, std::string& value, const std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::InputText(fmt::format("##{}", label).c_str(), &value);

    end_property();
    return modified;
}

void property(std::string_view label, const std::string& value, const std::string_view help_text)
{
    begin_property(label, help_text);

    ImGui::BeginDisabled();
    ImGui::InputText(fmt::format("##{}", label).c_str(), const_cast<char*>(value.c_str()), value.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::EndDisabled();

    end_property();
}

void Property(std::string_view label, std::string_view value, const std::string_view help_text)
{
    begin_property(label, help_text);

    ImGui::BeginDisabled();
    ImGui::InputText(fmt::format("##{}", label).c_str(), const_cast<char*>(value.data()), value.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::EndDisabled();

    end_property();
}

bool property_multiline(std::string_view label, std::string& value, const std::string_view help_text)
{
    bool modified = false;

    ImGui::TextUnformatted(label.data());
    if (!help_text.empty())
    {
        ImGui::SameLine();
        help_marker(help_text);
    }

    ImGui::NextColumn();
    ImGui::PushItemWidth(-1.f);

    modified = ImGui::InputTextMultiline(fmt::format("##{}", label).c_str(), &value);

    ImGui::PopItemWidth();
    ImGui::NextColumn();

    return modified;
}

bool property(std::string_view label, bool& value, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::Checkbox(fmt::format("##{}", label).c_str(), &value);

    end_property();
    return modified;
}

bool property(std::string_view label, int8_t& value, int8_t min, int8_t max, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::DragScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_S8, &value, 1.f, &min, &max);

    end_property();
    return modified;
}

bool property(std::string_view label, int16_t& value, int16_t min, int16_t max, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::DragScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_S16, &value, 1.f, &min, &max);

    end_property();
    return modified;
}

bool property(std::string_view label, int32_t& value, int32_t min, int32_t max, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::DragScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_S32, &value, 1.f, &min, &max);

    end_property();
    return modified;
}

bool property(std::string_view label, int64_t& value, int64_t min, int64_t max, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::DragScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_S64, &value, 1.f, &min, &max);

    end_property();
    return modified;
}

bool property(std::string_view label, uint8_t& value, uint8_t min, uint8_t max, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::DragScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_U8, &value, 1.f, &min, &max);

    end_property();
    return modified;
}

bool property(std::string_view label, uint16_t& value, uint16_t min, uint16_t max, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::DragScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_U16, &value, 1.f, &min, &max);

    end_property();
    return modified;
}

bool property(std::string_view label, uint32_t& value, uint32_t min, uint32_t max, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::DragScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_U32, &value, 1.f, &min, &max);

    end_property();
    return modified;
}

bool property(std::string_view label, uint64_t& value, uint64_t min, uint64_t max, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::DragScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_U64, &value, 1.f, &min, &max);

    end_property();
    return modified;
}

bool property_slider(std::string_view label, int& value, int min, int max, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::SliderInt(fmt::format("##{}", label).c_str(), &value, min, max);

    end_property();
    return modified;
}

bool property_input(std::string_view label, int8_t& value, int8_t step, int8_t step_fast, ImGuiInputTextFlags flag, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::InputScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_S8, &value, &step, &step_fast, nullptr, flag);

    end_property();
    return modified;
}

bool property_input(std::string_view label, int16_t& value, int16_t step, int16_t step_fast, ImGuiInputTextFlags flag, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::InputScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_S16, &value, &step, &step_fast, nullptr, flag);

    end_property();
    return modified;
}

bool property_input(std::string_view label, int32_t& value, int32_t step, int32_t step_fast, ImGuiInputTextFlags flag, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::InputScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_S32, &value, &step, &step_fast, nullptr, flag);

    end_property();
    return modified;
}

bool property_input(std::string_view label, int64_t& value, int64_t step, int64_t step_fast, ImGuiInputTextFlags flag, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::InputScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_S64, &value, &step, &step_fast, nullptr, flag);

    end_property();
    return modified;
}

bool property_input(std::string_view label, uint8_t& value, uint8_t step, uint8_t step_fast, ImGuiInputTextFlags flag, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::InputScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_U8, &value, &step, &step_fast, nullptr, flag);

    end_property();
    return modified;
}

bool property_input(std::string_view label, uint16_t& value, uint16_t step, uint16_t step_fast, ImGuiInputTextFlags flag, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::InputScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_U16, &value, &step, &step_fast, nullptr, flag);

    end_property();
    return modified;
}

bool property_input(std::string_view label, uint32_t& value, uint32_t step, uint32_t step_fast, ImGuiInputTextFlags flag, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::InputScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_U32, &value, &step, &step_fast, nullptr, flag);

    end_property();
    return modified;
}

bool property_input(std::string_view label, uint64_t& value, uint64_t step, uint64_t step_fast, ImGuiInputTextFlags flag, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::InputScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_U64, &value, &step, &step_fast, nullptr, flag);

    end_property();
    return modified;
}

bool property(std::string_view label, float& value, float delta, float min, float max, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::DragScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_Float, &value, delta, &min, &max);

    end_property();
    return modified;
}

bool property(std::string_view label, double& value, float delta, double min, double max, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::DragScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_Double, &value, delta, &min, &max);

    end_property();
    return modified;
}

bool property_slider(std::string_view label, float& value, float step, float step_fast, ImGuiInputTextFlags flag, std::string_view help_text)
{
    bool modified = false;
    begin_property(label, help_text);

    modified = ImGui::InputScalar(fmt::format("##{}", label).c_str(), ImGuiDataType_Float, &value, &step, &step_fast, nullptr, flag);

    end_property();
    return modified;
}

bool property(std::string_view label, glm::vec2& value, float delta, float min, float max, std::string_view help_text)
{
    begin_property(label, help_text);

    const bool modified = ImGui::DragFloat2(fmt::format("##{}", label).c_str(), glm::value_ptr(value), delta, min, max);

    end_property();
    return modified;
}

bool property(EditorContext& context, std::string_view label, glm::vec3& value, float delta, float min, float max, std::string_view help_text)\
{
    begin_property(label, help_text);

    const ImVec2 size(ImGui::GetContentRegionAvail().x - 8.0f, ImGui::GetFrameHeightWithSpacing());
    [[maybe_unused]] bool manually_edited = false;
    const bool modified = edit_vec3(
        context,
        std::format("##{0}", label).c_str(),
        size,
        0.0f,
        manually_edited,
        value,
        VectorAxisBits::None,
        delta,
        glm::vec3{min},
        glm::vec3{max}
    );

    end_property();
    return modified;
}

bool property(std::string_view label, glm::vec4& value, float delta, float min, float max, std::string_view help_text)
{
    begin_property(label, help_text);

    const bool modified = ImGui::DragFloat4(fmt::format("##{}", label).c_str(), glm::value_ptr(value), delta, min, max);

    end_property();
    return modified;
}

bool property_slider(std::string_view label, glm::vec2& value, float min, float max, std::string_view help_text)
{
    begin_property(label, help_text);

    const bool modified = ImGui::SliderFloat2(fmt::format("##{}", label).c_str(), glm::value_ptr(value), min, max);

    end_property();
    return modified;
}

bool property_slider(std::string_view label, glm::vec3& value, float min, float max, std::string_view help_text)
{
    begin_property(label, help_text);

    const bool modified = ImGui::SliderFloat3(fmt::format("##{}", label).c_str(), glm::value_ptr(value), min, max);

    end_property();
    return modified;
}

bool property_slider(std::string_view label, glm::vec4& value, float min, float max, std::string_view help_text)
{
    begin_property(label, help_text);

    const bool modified = ImGui::SliderFloat4(fmt::format("##{}", label).c_str(), glm::value_ptr(value), min, max);

    end_property();
    return modified;
}

bool property_color(std::string_view label, glm::vec3& value, std::string_view help_text)
{
    begin_property(label, help_text);

    const bool modified = ImGui::ColorEdit3(fmt::format("##{}", label).c_str(), glm::value_ptr(value));

    end_property();
    return modified;
}

bool property_color(std::string_view label, glm::vec4& value, std::string_view help_text)
{
    begin_property(label, help_text);

    const bool modified = ImGui::ColorEdit4(fmt::format("##{}", label).c_str(), glm::value_ptr(value));

    end_property();
    return modified;
}

std::pair<std::string, bool> resource_validity_and_name(ResourceReference<Resource>& reference, const PropertyResourceReferenceSettings&)
{
    std::string name = "Null";
    bool valid = false;

    const auto state = reference.get_state();
    switch (state)
    {
    case ResourceState::Loaded:
        {
            name = reference.get_resource_name().string;
            valid = true;
            break;
        }
    case ResourceState::Pending:
        {
            name = std::string(reference.get_resource_name().string) + " (pending)";
            valid = true;
            break;
        }
    case ResourceState::Unloaded:
        {
            name = std::string(reference.get_resource_name().string) + " (unloaded)";;
            valid = true;
            break;
        }
    case ResourceState::Missing:
        {
            name = std::string(reference.get_resource_name().string) + " (missing)";;
            break;
        }
    case ResourceState::Error:
        {
            name = std::string(reference.get_resource_name().string) + " (errored)";;
            break;
        }
    case ResourceState::Null:
    case ResourceState::Unknown:
        break;
    }

    return {name, valid};
}

int menu_item_icon_padding(const float icon_size)
{
    const float space_width = ImGui::CalcTextSize(" ").x;
    return static_cast<int>((icon_size + ImGui::GetStyle().ItemInnerSpacing.x * 2.f) / space_width) + 1;
}

bool menu_item_with_image(
    const vk::DescriptorSet image,
    const char* label,
    const char* shortcut,
    const bool selected,
    const bool enabled,
    const float icon_size
)
{
    const int num_spaces = menu_item_icon_padding(icon_size);

    // Build padded label: [spaces][label]
    char padded[256];
    IM_ASSERT(num_spaces + static_cast<int>(ImStrlen(label)) < static_cast<int>(sizeof(padded)));
    for (int i = 0; i < num_spaces; ++i)
        padded[i] = ' ';
    ImStrncpy(padded + num_spaces, label, static_cast<int>(sizeof(padded)) - num_spaces);

    const bool clicked = ImGui::MenuItem(padded, shortcut, selected, enabled);

    // Render at natural colour; only tint disabled items
    const ImU32 tint = enabled
                           ? ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text))
                           : ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

    // Draw the icon into the reserved icon column area
    const ImRect item_rect = get_item_rect();
    const float pad_y = (item_rect.GetHeight() - icon_size) * 0.5f;
    const ImVec2 icon_min{item_rect.Min.x + ImGui::GetStyle().ItemInnerSpacing.x, item_rect.Min.y + pad_y};
    const ImVec2 icon_max{icon_min.x + icon_size, icon_min.y + icon_size};

    ImGui::GetWindowDrawList()->AddImage(
        static_cast<VkDescriptorSet>(image),
        icon_min,
        icon_max,
        ImVec2(0.f, 0.f),
        ImVec2(1.f, 1.f),
        tint
    );

    return clicked;
}

bool begin_menu_with_image(vk::DescriptorSet image, const char* label, bool enabled, float icon_size)
{
    const int num_spaces = menu_item_icon_padding(icon_size);

    // Build padded label: [spaces][label]
    char padded[256];
    IM_ASSERT(num_spaces + static_cast<int>(ImStrlen(label)) < static_cast<int>(sizeof(padded)));
    for (int i = 0; i < num_spaces; ++i)
        padded[i] = ' ';
    ImStrncpy(padded + num_spaces, label, static_cast<int>(sizeof(padded)) - num_spaces);

    const bool clicked = ImGui::BeginMenu(padded, enabled);

    // Render at natural colour; only tint disabled items
    ImU32 tint = enabled
                     ? ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text))
                     : ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));


    // Draw the icon into the reserved icon column area
    const ImRect item_rect = get_item_rect();
    const float pad_y = (item_rect.GetHeight() - icon_size) * 0.5f;
    const ImVec2 icon_min{item_rect.Min.x + ImGui::GetStyle().ItemInnerSpacing.x, item_rect.Min.y + pad_y};
    const ImVec2 icon_max{icon_min.x + icon_size, icon_min.y + icon_size};

    ImGui::GetWindowDrawList()->AddImage(
        static_cast<VkDescriptorSet>(image),
        icon_min,
        icon_max,
        ImVec2(0.f, 0.f),
        ImVec2(1.f, 1.f),
        tint
    );

    return clicked;
}

void underline(const bool full_width, const float offset_x, const float offset_y)
{
    const ImU32 color = ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Separator]);
    underline(color, full_width, offset_x, offset_y);
}

void underline(const ImU32 color, const bool full_width, const float offset_x, const float offset_y)
{
    if (full_width)
    {
        if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr)
            ImGui::PushColumnsBackground();
        else if (ImGui::GetCurrentTable() != nullptr)
            ImGui::TablePushBackgroundChannel();
    }

    const float width = full_width ? ImGui::GetWindowWidth() : ImGui::GetContentRegionAvail().x;
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(cursor.x + offset_x, cursor.y + offset_y),
        ImVec2(cursor.x + width, cursor.y + offset_y),
        color,
        1.0f
    );

    if (full_width)
    {
        if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr)
            ImGui::PopColumnsBackground();
        else if (ImGui::GetCurrentTable() != nullptr)
            ImGui::TablePopBackgroundChannel();
    }
}

bool navigated_to()
{
    ImGuiContext* g = ImGui::GetCurrentContext();
    return g->NavJustMovedToId == g->LastItemData.ID;
}
}
