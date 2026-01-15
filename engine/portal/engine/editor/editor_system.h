//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/engine/components/base.h"
#include "portal/engine/components/relationship.h"
#include "portal/engine/components/transform.h"
#include "portal/engine/ecs/system.h"

namespace portal
{

/**
 * @brief ECS system that renders the editor GUI panels.
 *
 * EditorGuiSystem renders the various editor panels using ImGui:
 * - Scene graph hierarchy view
 * - Entity details/inspector panel
 * - Performance statistics
 * - Editor controls
 */
class EditorGuiSystem : public ecs::System<EditorGuiSystem, ecs::Owns<NameComponent>, ecs::Views<RelationshipComponent>, ecs::Views<
                                               TransformComponent>>
{
public:
    /** @brief Main execution entry point, renders all editor panels. */
    static void execute(ecs::Registry& registry, FrameContext& frame);

    /** @brief Renders the scene graph hierarchy panel. */
    static void print_scene_graph(ecs::Registry& registry, FrameContext& frame);

    /** @brief Renders editor control widgets. */
    static void print_controls(ecs::Registry& registry);

    /** @brief Renders performance statistics panel. */
    static void print_stats_block(ecs::Registry& registry, FrameContext& frame);

    /** @brief Renders the entity details/inspector panel. */
    static void print_details_panel(ecs::Registry& registry, const FrameContext& frame);

    [[nodiscard]] static StringId get_name() { return STRING_ID("Editor System"); };
};

} // portal
