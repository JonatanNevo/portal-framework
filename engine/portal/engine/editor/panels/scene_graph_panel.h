//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <imgui.h>
#include <imgui_internal.h>

#include "panel.h"
#include "portal/engine/ecs/entity.h"

namespace portal
{

class SceneGraphPanel final : public Panel
{
public:
    void on_gui_render(EditorContext& editor_context, FrameContext& frame_context, bool& is_open) override;

private:
    void draw_entity_node(EditorContext& editor_context, const Entity& entity, const Entity& scene_entity);
    void draw_entity_create_menu(Entity parent);

    bool name_search_recursive(const Entity& entity, uint32_t search_depth, uint32_t current_depth = 0);

private:
    ImRect window_bounds;

    std::string search_string;
    bool activate_search_widget = false;

    bool window_focused = false;

    bool shift_selection_running = false;
    int32_t first_selected_row = -1;
    int32_t last_selected_row = -1;
};
} // portal