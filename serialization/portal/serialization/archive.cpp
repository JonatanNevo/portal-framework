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
        if (prop.type == reflection::PropertyType::object)
        {
            if (prop.container_type == reflection::PropertyContainerType::object)
            {
                auto* other_object = other.get_object(name);
                auto* child = get_object(name);
                if (child)
                    child->update(*other_object);
                else
                    create_child(name)->update(*other_object);
            }
            if (prop.container_type == reflection::PropertyContainerType::array)
            {
                auto* other_objects = prop.value.as<ArchiveObject*>();

                // Check if we already have this property as an array
                auto& existing_prop = get_property_from_map(name);
                if (existing_prop.type == reflection::PropertyType::object &&
                    existing_prop.container_type == reflection::PropertyContainerType::array &&
                    existing_prop.elements_number == prop.elements_number)
                {
                    // Update each element in the existing array
                    auto* our_objects = existing_prop.value.as<ArchiveObject*>();
                    for (size_t i = 0; i < prop.elements_number; ++i)
                    {
                        our_objects[i].update(other_objects[i]);
                    }
                }
                else
                {
                    // Create a new array by copying from other
                    Buffer buffer = Buffer::allocate(prop.elements_number * sizeof(ArchiveObject));
                    auto* new_objects = buffer.as<ArchiveObject*>();

                    for (size_t i = 0; i < prop.elements_number; ++i)
                    {
                        new (new_objects + i) ArchiveObject(other_objects[i]);
                    }

                    property_map[name] = reflection::Property{
                        .value = std::move(buffer),
                        .type = reflection::PropertyType::object,
                        .container_type = reflection::PropertyContainerType::array,
                        .elements_number = prop.elements_number
                    };
                }
            }
        }
        else
        {
            property_map[name] = reflection::Property{
                .value = Buffer::copy(prop.value),
                .type = prop.type,
                .container_type = prop.container_type,
                .elements_number = prop.elements_number
            };
        }
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

void ArchiveObject::add_property(const PropertyName& name, const StringId& string_id)
{
    add_property(name, string_id.string);
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

bool ArchiveObject::get_property(const PropertyName& name, StringId& out)
{
    std::string string;
    if (!get_property<std::string>(name, string))
        return false;

    out = STRING_ID(string);
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

ArchiveObject* ArchiveObject::get_object(PropertyName name) const
{
#ifdef PORTAL_DEBUG
    if (!property_map.contains(std::string{name}))
        return nullptr;
#else
    if (!property_map.contains(name))
        return nullptr;
#endif


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

const reflection::Property& ArchiveObject::get_property_from_map(PropertyName name) const
{
#ifdef PORTAL_DEBUG
    return property_map.at(std::string{name});
#else
    return property_map.at(name);
#endif
}
}
