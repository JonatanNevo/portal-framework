//
// Created by Jonatan Nevo on 22/02/2025.
//

#pragma once

#include <vector>

#include "portal/serialization/property.h"

namespace portal::serialization
{
class Deserializer
{
public:
    virtual ~Deserializer() = default;
    virtual void deserialize() = 0;

    template <std::integral T>
    T get_property(const std::string& name)
    {
        if (!properties.contains(name))
            throw std::runtime_error("Property not found");

        const auto& property = properties.at(name);
        if (property.type != PropertyType::integer)
            throw std::runtime_error("Property type mismatch");

        if (property.value.size != sizeof(T))
            throw std::runtime_error("Property size mismatch");

        return *static_cast<T*>(property.value.data);

    }

    template <std::floating_point T>
    T get_property(const std::string& name)
    {
        if (!properties.contains(name))
            throw std::runtime_error("Property not found");

        const auto& property = properties.at(name);
        if (property.type != PropertyType::floating)
            throw std::runtime_error("Property type mismatch");

        if (property.value.size != sizeof(T))
            throw std::runtime_error("Property size mismatch");

        return *static_cast<T*>(property.value.data);
    }

    template <Vector T>
    T get_property(const std::string& name)
    {
        if (!properties.contains(name))
            throw std::runtime_error("Property not found");

        const auto& property = properties.at(name);

        if (property.container_type != PropertyContainerType::array)
            throw std::runtime_error("Property container type mismatch");

        auto array_length = property.value.size / sizeof(typename T::value_type);
        auto* data = static_cast<typename T::value_type*>(property.value.data);
        return T(data, data + array_length);
    }

    template <String T>
    T get_property(const std::string& name)
    {
        if (!properties.contains(name))
            throw std::runtime_error("Property not found");

        const auto& property = properties.at(name);

        if (property.container_type != PropertyContainerType::array)
            throw std::runtime_error("Property container type mismatch");

        const auto string_length = property.value.size / sizeof(typename T::value_type) - 1;
        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        return T(data, string_length);
    }

    template <GlmVec1 T>
    T get_property(const std::string& name)
    {
        if (!properties.contains(name))
            throw std::runtime_error("Property not found");

        const auto& property = properties.at(name);

        if (property.container_type != PropertyContainerType::vector)
            throw std::runtime_error("Property container type mismatch");

        return T(*static_cast<typename T::value_type*>(property.value.data));
    }

    template <GlmVec2 T>
    T get_property(const std::string& name)
    {
        if (!properties.contains(name))
            throw std::runtime_error("Property not found");

        const auto& property = properties.at(name);

        if (property.container_type != PropertyContainerType::vector)
            throw std::runtime_error("Property container type mismatch");

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        return T(data[0], data[1]);
    }

    template <GlmVec3 T>
    T get_property(const std::string& name)
    {
        if (!properties.contains(name))
            throw std::runtime_error("Property not found");

        const auto& property = properties.at(name);

        if (property.container_type != PropertyContainerType::vector)
            throw std::runtime_error("Property container type mismatch");

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        return T(data[0], data[1], data[2]);
    }

    template <GlmVec4 T>
    T get_property(const std::string& name)
    {
        if (!properties.contains(name))
            throw std::runtime_error("Property not found");

        const auto& property = properties.at(name);

        if (property.container_type != PropertyContainerType::vector)
            throw std::runtime_error("Property container type mismatch");

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        return T(data[0], data[1], data[2], data[3]);
    }
protected:
    std::map<std::string, Property> properties;
};


class OrderedDeserializer: public Deserializer
{
public:
    void deserialize() override = 0;

    template <typename T>
    T get_property()
    {
        return Deserializer::get_property<T>(std::to_string(counter++));
    }

protected:
    size_t counter = 0;

};
}
