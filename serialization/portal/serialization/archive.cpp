//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//


#include "archive.h"

namespace portal
{

ArchiveObject* ArchiveObject::create_child(PropertyName name)
{
    reflection::Property child_property{
        Buffer::create<ArchiveObject>(),
        reflection::PropertyType::object,
        reflection::PropertyContainerType::object,
        1
    };
    const auto& new_node = add_property_to_map(name, std::move(child_property));
    return new_node.value.as<ArchiveObject*>();
}

reflection::Property& ArchiveObject::add_property_to_map(PropertyName name, reflection::Property&& property)
{
    auto& prop = property_map[std::string(name)];
    PORTAL_ASSERT(
        prop.type == reflection::PropertyType::invalid || prop.type == property.type,
        "Property {} already exists with a different type",
        name
        );
    prop = std::move(property);
    return prop;
}

ArchiveObject* ArchiveObject::get_object(PropertyName name) {
    const auto& prop = property_map[std::string(name)];
    if (prop.type == reflection::PropertyType::invalid || prop.container_type != reflection::PropertyContainerType::object) {
        LOG_ERROR_TAG("Serialization", "Property {} is not an object", name);
        return nullptr;
    }
    return prop.value.as<ArchiveObject*>();
}


}
