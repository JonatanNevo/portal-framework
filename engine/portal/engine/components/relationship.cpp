//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "relationship.h"

#include "base.h"

namespace portal
{
void RelationshipComponent::archive(ArchiveObject& archive) const
{
    archive.add_property("parent", parent.get_component<NameComponent>().name);
}

RelationshipComponent RelationshipComponent::dearchive(ArchiveObject& archive, Entity entity, ecs::Registry& ecs_reg)
{
    StringId parent_name;
    archive.get_property("parent", parent_name);

    const auto parent = ecs_reg.find_by_name(parent_name);
    if (parent.has_value())
        entity.set_parent(parent.value());

    return entity.get_component<RelationshipComponent>();
}

void RelationshipComponent::serialize(Serializer& serialize) const
{
    serialize.add_value(parent.get_component<NameComponent>().name);
}

RelationshipComponent RelationshipComponent::deserialize(Deserializer& deserializer, Entity entity, ecs::Registry& ecs_reg)
{
    StringId parent_name;
    deserializer.get_value(parent_name);

    const auto parent = ecs_reg.find_by_name(parent_name);
    if (parent.has_value())
        entity.set_parent(parent.value());

    return entity.get_component<RelationshipComponent>();
}
}
