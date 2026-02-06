//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "panel.h"

#include <vulkan/vulkan.hpp>

#include "portal/engine/editor/editor_icons.h"
#include "portal/engine/imgui/theme/editor_theme.h"
#include "portal/engine/renderer/vulkan/image/vulkan_texture.h"

namespace portal
{

// TODO: is this really a panel?
class WindowTitlebar final : public Panel
{
public:
    WindowTitlebar(EditorContext& context);
    void on_gui_render(EditorContext& editor_context, FrameContext& frame_context) override;
    [[nodiscard]] float get_height() const { return height; }

private:
    void draw_menubar(EditorContext& editor_context);

private:
    float height = 0;
    bool titlebar_hovered = false;

    ImVec4 active_color;
    ImVec4 target_color;
    ImVec4 previous_color;
    bool animate_titlebar_color = true;
};
} // portal
