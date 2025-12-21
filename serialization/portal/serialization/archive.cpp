//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//


#include "archive.h"

namespace portal
{
void ArchiveObject::update(const ArchiveObject& other)
{
    for (auto& [name, prop] : other.property_map)
    {
        property_map[name] = reflection::Property{
            .value = Buffer::copy(prop.value),
            .type = prop.type,
            .container_type = prop.container_type,
            .elements_number = prop.elements_number
        };
    }
}

void ArchiveObject::add_property(const PropertyName& name, const char* t)
{
    const auto len = strlen(t);
    add_property_to_map(
        name,
        reflection::Property{
            Buffer{const_cast<void*>(static_cast<const void*>(t)), (len * sizeof(char)) + 1},
            reflection::PropertyType::character,
            reflection::PropertyContainerType::null_term_string,
            len + 1
        }
    );
}

void ArchiveObject::add_property(const PropertyName& name, const std::filesystem::path& t)
{
    add_property(name, t.string());
}

bool ArchiveObject::get_property(const PropertyName& name, std::filesystem::path& out)
{
    std::string string;
    if (!get_property<std::string>(name, string))
        return false;

    // TODO: make absolute?
    out = std::filesystem::path(string);
    return true;
}

ArchiveObject* ArchiveObject::create_child(const PropertyName name)
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
    auto& prop = get_property_from_map(name);
    PORTAL_ASSERT(
        prop.type == reflection::PropertyType::invalid || prop.type == property.type,
        "Property {} already exists with a different type",
        name
    );
    prop = std::move(property);
    return prop;
}

ArchiveObject* ArchiveObject::get_object(PropertyName name)
{
    const auto& prop = get_property_from_map(name);
    if (prop.type == reflection::PropertyType::invalid || prop.container_type != reflection::PropertyContainerType::object)
    {
        return nullptr;
    }
    return prop.value.as<ArchiveObject*>();
}

reflection::Property& ArchiveObject::get_property_from_map(const PropertyName name)
{
#ifdef PORTAL_DEBUG
    return property_map[std::string{name}];
#else
    return property_map[name];
#endif
}
}
