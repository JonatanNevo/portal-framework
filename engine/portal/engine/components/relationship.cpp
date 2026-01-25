//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "relationship.h"

#include "base.h"

namespace portal
{
void RelationshipComponent::archive(ArchiveObject& archive, Entity entity, ecs::Registry&) const
{
    archive.add_property("parent", parent.get_component<NameComponent>().name);

    std::vector<std::string_view> children_names;
    for (auto child : entity.children())
    {
        children_names.push_back(child.get_component<NameComponent>().name.string);
    }
    archive.add_property("children", children_names);
}

RelationshipComponent RelationshipComponent::dearchive(ArchiveObject& archive, Entity entity, ecs::Registry& ecs_reg)
{
    StringId parent_name;
    archive.get_property("parent", parent_name);

    std::vector<StringId> children;
    archive.get_property("children", children);

    auto parent = ecs_reg.find_by_name(parent_name);
    if (parent.has_value())
        entity.set_parent(parent.value());

    for (auto child_name : children)
    {
        auto child = ecs_reg.find_by_name(child_name);
        if (child.has_value())
            child.value().set_parent(entity);
    }

    return entity.get_component<RelationshipComponent>();
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
