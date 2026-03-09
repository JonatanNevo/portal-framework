//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include <vulkan/vulkan.hpp>
#include <enchantum/enchantum.hpp>

#include "imgui_fonts.h"
#include "imgui_scoped.h"
#include "theme/serializers.h"
#include "portal/core/strings/string_id.h"
#include "widgets/resource_search.h"

namespace portal
{
struct EditorContext;
}

namespace portal::imgui
{
//=========================================================================================

inline ImVec4 rgb(const float r, const float g, const float b, const float a = 255.f)
{
    return ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
}

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
ImColor color_with_multiplied_saturation(const ImColor& color, float multiplier);

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

const char* generate_id();
const char* generate_label_id(std::string_view label);

void push_id();
void pop_id();

//=========================================================================================
void underline(bool full_width = false, float offset_x = 0.f, float offset_y = -1.f);
void underline(ImU32 color, bool full_width = false, float offset_x = 0.f, float offset_y = -1.f);

//=========================================================================================
void help_marker(std::string_view description);

void begin_property_grid(size_t columns = 2);
void end_property_grid();

void begin_property(std::string_view label, std::string_view help_text = "");
void end_property();

// TODO: use reflection and portal::Property

// string value
bool property(std::string_view label, std::string& value, std::string_view help_text = "");
void property(std::string_view label, const std::string& value, std::string_view help_text = "");
void Property(std::string_view label, std::string_view value, std::string_view help_text = "");
bool property_multiline(std::string_view label, std::string& value, std::string_view help_text = "");

// bool value
bool property(std::string_view label, bool& value, std::string_view help_text = "");

// int values
bool property(std::string_view label, int8_t& value, int8_t min = 0, int8_t max = 0, std::string_view help_text = "");
bool property(std::string_view label, int16_t& value, int16_t min = 0, int16_t max = 0, std::string_view help_text = "");
bool property(std::string_view label, int32_t& value, int32_t min = 0, int32_t max = 0, std::string_view help_text = "");
bool property(std::string_view label, int64_t& value, int64_t min = 0, int64_t max = 0, std::string_view help_text = "");
bool property(std::string_view label, uint8_t& value, uint8_t min = 0, uint8_t max = 0, std::string_view help_text = "");
bool property(std::string_view label, uint16_t& value, uint16_t min = 0, uint16_t max = 0, std::string_view help_text = "");
bool property(std::string_view label, uint32_t& value, uint32_t min = 0, uint32_t max = 0, std::string_view help_text = "");
bool property(std::string_view label, uint64_t& value, uint64_t min = 0, uint64_t max = 0, std::string_view help_text = "");
bool property_slider(std::string_view label, int& value, int min, int max, std::string_view help_text = "");
bool property_input(
    std::string_view label,
    int8_t& value,
    int8_t step = 1,
    int8_t step_fast = 1,
    ImGuiInputTextFlags flag = 0,
    std::string_view help_text = ""
);
bool property_input(
    std::string_view label,
    int16_t& value,
    int16_t step = 1,
    int16_t step_fast = 1,
    ImGuiInputTextFlags flag = 0,
    std::string_view help_text = ""
);
bool property_input(
    std::string_view label,
    int32_t& value,
    int32_t step = 1,
    int32_t step_fast = 1,
    ImGuiInputTextFlags flag = 0,
    std::string_view help_text = ""
);
bool property_input(
    std::string_view label,
    int64_t& value,
    int64_t step = 1,
    int64_t step_fast = 1,
    ImGuiInputTextFlags flag = 0,
    std::string_view help_text = ""
);
bool property_input(
    std::string_view label,
    uint8_t& value,
    uint8_t step = 1,
    uint8_t step_fast = 1,
    ImGuiInputTextFlags flag = 0,
    std::string_view help_text = ""
);
bool property_input(
    std::string_view label,
    uint16_t& value,
    uint16_t step = 1,
    uint16_t step_fast = 1,
    ImGuiInputTextFlags flag = 0,
    std::string_view help_text = ""
);
bool property_input(
    std::string_view label,
    uint32_t& value,
    uint32_t step = 1,
    uint32_t step_fast = 1,
    ImGuiInputTextFlags flag = 0,
    std::string_view help_text = ""
);
bool property_input(
    std::string_view label,
    uint64_t& value,
    uint64_t step = 1,
    uint64_t step_fast = 1,
    ImGuiInputTextFlags flag = 0,
    std::string_view help_text = ""
);

// floats
bool property(std::string_view label, float& value, float delta = 0.1f, float min = 0.f, float max = 0.f, std::string_view help_text = "");
bool property(std::string_view label, double& value, float delta = 0.1f, double min = 0.f, double max = 0.f, std::string_view help_text = "");
bool property_slider(
    std::string_view label,
    float& value,
    float step = 1,
    float step_fast = 1,
    ImGuiInputTextFlags flag = 0,
    std::string_view help_text = ""
);

// vectors
bool property(std::string_view label, glm::vec2& value, float delta = 0.1f, float min = 0.f, float max = 0.f, std::string_view help_text = "");
bool property(
    EditorContext& context,
    std::string_view label,
    glm::vec3& value,
    float delta = 0.1f,
    float min = 0.f,
    float max = 0.f,
    std::string_view help_text = ""
);
bool property(std::string_view label, glm::vec4& value, float delta = 0.1f, float min = 0.f, float max = 0.f, std::string_view help_text = "");
bool property_slider(std::string_view label, glm::vec2& value, float min, float max, std::string_view help_text = "");
bool property_slider(std::string_view label, glm::vec3& value, float min, float max, std::string_view help_text = "");
bool property_slider(std::string_view label, glm::vec4& value, float min, float max, std::string_view help_text = "");

// color
bool property_color(std::string_view label, glm::vec3& value, std::string_view help_text = "");
bool property_color(std::string_view label, glm::vec4& value, std::string_view help_text = "");


// widgets
bool property_button(std::string_view label, std::string_view button_text, std::string_view help_text = "");

bool property_radio(
    std::string_view label,
    int& choice,
    const std::map<int, const std::string_view>& options,
    std::string_view help_text = "",
    const std::map<int, const std::string_view>& options_help_text = {}
);

template <typename Enum> requires std::is_enum_v<Enum>
bool property_dropdown(const std::string_view label, Enum& selected, const std::string_view help_text = "")
{
    auto selected_index = std::to_underlying(selected);
    std::string_view current = enchantum::to_string(selected);

    begin_property(label, help_text);

    bool modified = false;
    if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
        current = "---";

    const auto string_id = "##" + std::string(label);
    if (ImGui::BeginCombo(string_id.c_str(), current.data()))
    {
        for (auto& [value, name] : enchantum::entries_generator<Enum>)
        {
            const bool is_selected = selected == value;
            if (ImGui::Selectable(name.data(), is_selected))
            {
                current = name;
                selected = value;
                modified = true;
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    end_property();
    return modified;
}

enum class ResourceReferenceErrors
{
    None,
    InvalidMetadata
};

struct PropertyResourceReferenceSettings
{
    bool advance_to_next_column = true;
    bool no_item_spacing_after_label = false;
    float width_offset = 0.f;
    ImVec4 button_label_color = ImGui::GetStyle().Colors[ImGuiCol_Text];
    ImVec4 button_label_color_error = rgb(219, 72, 115);
};

std::pair<std::string, bool> resource_validity_and_name(ResourceReference<Resource>& reference, const PropertyResourceReferenceSettings& settings);

// resources
template <ResourceConcept T, typename ConversionType, typename Func>
std::expected<bool, ResourceReferenceErrors> property_resource_reference_with_conversion(
    EditorContext& context,
    std::string_view label,
    ResourceReference<T>& out_ref,
    Func&& conversion_func,
    std::string_view help_text = "",
    const PropertyResourceReferenceSettings& settings = {}
)
{
    bool success = false;

    begin_property(label, help_text);

    auto original_button_align = ImGui::GetStyle().ButtonTextAlign;
    ImGui::GetStyle().ButtonTextAlign = {0.0f, 0.5f};
    float width = ImGui::GetContentRegionAvail().x - settings.width_offset;
    constexpr auto item_height = 28.f;

    push_id();


    auto base_reference = out_ref.template cast<Resource>();
    auto [name, valid] = resource_validity_and_name(base_reference, settings);

    if ((GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0)
        name = "---";

    // property_resource_reference_with_conversion could be called multiple times in same "context"
    // and so we need a unique id for the asset search popup each time.
    // notes
    // - don't use generate_id(), that's inviting id clashes, which would be super confusing.
    // - don't store return from generate_label_id() in a const char* here. Because its pointing to an internal
    //   buffer which may get overwritten by the time you want to use it later on.
    std::string resource_search_popup_id = generate_label_id("property_resource_reference_with_conversion_popup");
    {
        ScopedColor button_label(ImGuiCol_Text, valid ? settings.button_label_color : settings.button_label_color_error);
        ImGui::Button(generate_label_id(name.data()), {width, item_height});

        const bool is_hovered = ImGui::IsItemHovered();
        if (is_hovered)
        {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                // TODO: open resource editor
            }
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                ImGui::OpenPopup(resource_search_popup_id.c_str());
            }
        }
    }

    ImGui::GetStyle().ButtonTextAlign = original_button_align;

    bool clear = false;
    if (resource_search_popup(
        context,
        resource_search_popup_id,
        base_reference,
        &clear,
        "Search Resources",
        ImVec2{250.f, 250.f},
        {
            utils::to_resource_type<T>(),
            utils::to_resource_type<ConversionType>()
        }
    ))
    {
        if (clear)
        {
            out_ref = {};
            base_reference = {};
            success = true;
        }
    }
    pop_id();

    if (!is_item_disabled())
    {
        StringId resource_id = base_reference.get_resource_id();
        if (ImGui::BeginDragDropTarget())
        {
            auto data = ImGui::AcceptDragDropPayload("resource_payload");
            if (data)
                resource_id = *static_cast<StringId*>(data->Data);

            ImGui::EndDragDropTarget();
        }

        if (resource_id != INVALID_STRING_ID && resource_id != out_ref.get_resource_id())
        {
            auto ref = context.resource_registry.get<T>(resource_id);
            if (ref.is_valid())
            {
                if (ref.get_resource_type() == utils::to_resource_type<T>())
                {
                    out_ref = ref;
                    success = true;
                }
                else if (ref.get_resource_type() == utils::to_resource_type<ConversionType>())
                {
                    conversion_func(ref.template cast<ConversionType>());
                    success = true;
                }
            }
            else
            {
                end_property();
                return std::unexpected(ResourceReferenceErrors::InvalidMetadata);
            }
        }
    }

    end_property();

    return success;
}

//=========================================================================================

//=========================================================================================
// Returns the number of space characters menu_item_with_image pads onto the label to
// reserve the icon column.  Use this to pad plain labels (e.g. submenu headers) so they
// align with icon'd items in the same menu.
int menu_item_icon_padding(float icon_size = 16.f);

// Calls ImGui::MenuItem with the given label, then overlays an image icon into the item's
// icon column.  The icon is rendered at its natural colour; only disabled items are tinted
// with ImGuiCol_TextDisabled.
bool menu_item_with_image(
    vk::DescriptorSet image,
    const char* label,
    const char* shortcut = nullptr,
    bool selected = false,
    bool enabled = true,
    float icon_size = 16.f
);

bool begin_menu_with_image(
    vk::DescriptorSet image,
    const char* label,
    bool enabled = true,
    float icon_size = 16.f
);

//=========================================================================================
bool navigated_to();
}
