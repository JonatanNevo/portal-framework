//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/core/flags.h"
#include "portal/engine/editor/editor_context.h"

namespace portal::imgui
{
enum class VectorAxisBits: uint8_t
{
    None = 0b0000,
    X    = 0b0001,
    Y    = 0b0010,
    Z    = 0b0100,
    W    = 0b1000
};

using VectorAxis = Flags<VectorAxisBits>;

bool edit_vec3(
    EditorContext& context,
    std::string_view label,
    ImVec2 size,
    float reset_value,
    bool& manually_edited,
    glm::vec3 value,
    VectorAxis render_multi_select_axes = VectorAxisBits::None,
    float speed = 1.f,
    glm::vec3 v_min = glm::zero<glm::vec3>(),
    glm::vec3 v_max = glm::one<glm::vec3>(),
    const char* format = "%.2f",
    ImGuiSliderFlags flags = ImGuiSliderFlags_None
);
}

template <>
struct portal::FlagTraits<portal::imgui::VectorAxisBits>
{
    using enum imgui::VectorAxisBits;

    static constexpr bool is_bitmask = true;
    static constexpr auto all_flags = X | Y | Z | W;
};
