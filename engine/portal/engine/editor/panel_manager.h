//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <imgui.h>

#include "editor_context.h"
#include "panels/panel.h"
#include "portal/engine/components/base.h"
#include "portal/engine/components/relationship.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/ecs/system.h"
#include "portal/engine/imgui/dialogs.h"
#include "portal/engine/imgui/imgui_scoped.h"
#include "portal/engine/imgui/utils.h"
#include "portal/third_party/font_awsome/IconsFontAwesome6.h"

namespace portal
{
class PanelManager
{
public:
    /** @brief Main execution entry point, renders all editor panels. */
    void on_gui_render(EditorContext& editor_context, FrameContext& frame);

    /** @brief Renders the scene graph hierarchy panel. */
    void print_scene_graph(ecs::Registry& registry, const FrameContext& frame);

    /** @brief Renders editor control widgets. */
    void print_controls(ecs::Registry& registry);

    /** @brief Renders performance statistics panel. */
    void print_stats_block(ecs::Registry& registry, FrameContext& frame);

    template<typename T, typename... Args>
    void add_panel(Args&&... args)
    {
        panels.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

private:
    std::vector<std::unique_ptr<Panel>> panels;

};
} // portal
