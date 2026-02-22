//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <vulkan/vulkan.hpp>
#include <imgui.h>

namespace portal
{
bool tree_node_with_icon(
    vk::DescriptorSet icon,
    vk::DescriptorSet icon_opened,
    ImGuiID id,
    ImGuiTreeNodeFlags flags,
    const char* label,
    const char* label_end,
    ImColor icon_tint = IM_COL32_WHITE
);

bool tree_node_with_icon(
    vk::DescriptorSet icon,
    vk::DescriptorSet icon_opened,
    const void* ptr_id,
    ImGuiTreeNodeFlags flags,
    ImColor icon_tint,
    const char* fmt,
    ...
);

bool tree_node_with_icon(
    vk::DescriptorSet icon,
    vk::DescriptorSet icon_opened,
    const char* label,
    ImGuiTreeNodeFlags flags,
    ImColor icon_tint = IM_COL32_WHITE
);
}
