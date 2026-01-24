//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/engine/ecs/entity.h>
#include "register_component.h"

namespace portal
{
struct RelationshipComponent
{
    // Children
    size_t children;
    Entity first = null_entity;
    Entity prev = null_entity;
    Entity next = null_entity;

    // Parent
    Entity parent = null_entity;

    void archive(ArchiveObject& archive, Entity entity, ecs::Registry& registry) const;
    static RelationshipComponent dearchive(ArchiveObject& archive, Entity entity, ecs::Registry& registry);

    void serialize(Serializer& serialize) const;
    static RelationshipComponent deserialize(Deserializer& archive);
};

REGISTER_COMPONENT(RelationshipComponent);
}
