//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "relationship.h"

namespace portal
{
void RelationshipComponent::archive(ArchiveObject& archive, const Entity entity, ecs::Registry&) const
{
    archive.add_property("parent", parent.get_component<NameComponent>().name);

    std::vector<std::string_view> children;
    for (auto child : entity.children())
    {
        children.push_back(child.get_component<NameComponent>().name.string);
    }
    archive.add_property("children", children);
}

RelationshipComponent RelationshipComponent::dearchive(ArchiveObject& archive, Entity, ecs::Registry&)
{
    StringId parent_name;
    archive.get_property("parent", parent_name);

    std::vector<StringId> children;
    archive.get_property("children", children);

    // TODO: reconstruct list
    return RelationshipComponent{

    };
}

void RelationshipComponent::serialize(Serializer&) const
{
    throw std::runtime_error("Not implemented");
}

RelationshipComponent RelationshipComponent::deserialize(Deserializer&)
{
    throw std::runtime_error("Not implemented");
}
}
