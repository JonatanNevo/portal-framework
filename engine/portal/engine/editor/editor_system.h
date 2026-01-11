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
class EditorGuiSystem : public ecs::System<EditorGuiSystem, ecs::Owns<NameComponent>, ecs::Views<RelationshipComponent>, ecs::Views<
                                               TransformComponent>>
{
public:
    static void execute(ecs::Registry& registry);

    static void print_cameras(ecs::Registry& registry);
    static void print_scene_graph(ecs::Registry& registry);
    static void print_controls(ecs::Registry& registry);

    [[nodiscard]] static StringId get_name() { return STRING_ID("Editor System"); };
};
} // portal
