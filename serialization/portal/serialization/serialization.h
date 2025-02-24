//
// Created by Jonatan Nevo on 22/02/2025.
//

#pragma once

#include <vector>
#include "portal/core/log.h"
#include "portal/serialization/property.h"

namespace portal
{

class Serializer
{
public:
    virtual ~Serializer() = default;
    virtual void serialize() = 0;

    template <typename T> requires std::integral<T> || std::floating_point<T>
    void add_value(T& t)
    {
        properties.emplace_back(
            {
                Buffer{&t, sizeof(T)},
                serialization::get_property_type<T>(),
                serialization::PropertyContainerType::scalar,
                1
            }
            );
    }

    template <serialization::Vector T>
    void add_value(T& t)
    {
        properties.emplace_back(
            {
                Buffer{t.data(), t.size() * sizeof(typename T::value_type)},
                serialization::get_property_type<typename T::value_type>(),
                serialization::PropertyContainerType::array,
                t.size()
            }
            );
    }

    template <serialization::String T>
    void add_value(T& t)
    {
        properties.emplace_back(
            {
                Buffer{t.data(), (t.size() * sizeof(typename T::value_type)) + 1},
                serialization::PropertyType::character,
                serialization::PropertyContainerType::null_term_string,
                t.size() + 1
            }
            );
    }

    template <serialization::GlmVec1 T>
    void add_value(T& t)
    {
        properties.emplace_back(
            {
                Buffer{&t.x, sizeof(typename T::value_type)},
                serialization::get_property_type<typename T::value_type>(),
                serialization::PropertyContainerType::vec1,
                1
            }
            );
    }

    template <serialization::GlmVec2 T>
    void add_value(T& t)
    {
        properties.emplace_back(
            {
                Buffer{&t.x, 2 * sizeof(typename T::value_type)},
                serialization::get_property_type<typename T::value_type>(),
                serialization::PropertyContainerType::vec2,
                2
            }
            );
    }

    template <serialization::GlmVec3 T>
    void add_value(T& t)
    {
        properties.emplace_back(
            {
                Buffer{&t.x, 3 * sizeof(typename T::value_type)},
                serialization::get_property_type<typename T::value_type>(),
                serialization::PropertyContainerType::vec3,
                3
            }
            );
    }

    template <serialization::GlmVec4 T>
    void add_value(T& t)
    {
        properties.emplace_back(
            {
                Buffer{&t.x, 4 * sizeof(typename T::value_type)},
                serialization::get_property_type<typename T::value_type>(),
                serialization::PropertyContainerType::vec4,
                4
            }
            );
    }

protected:
    std::vector<serialization::Property> properties;
};


class Deserializer
{
public:
    virtual ~Deserializer() = default;
    virtual void deserialize() = 0;

    template <typename T> requires std::integral<T> || std::floating_point<T>
    T get_property(const std::string& name)
    {
        PORTAL_CORE_ASSERT(properties.contains(name), "Property {} not found", name);

        const auto& property = properties.at(name);

        PORTAL_CORE_ASSERT(property.type == serialization::get_property_type<T>(), "Property {} type mismatch", name);

        return *static_cast<T*>(property.value.data);
    }

    template <serialization::Vector T>
    T get_property(const std::string& name)
    {
        PORTAL_CORE_ASSERT(properties.contains(name), "Property {} not found", name);

        const auto& property = properties.at(name);

        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::array, "Property {} container type mismatch", name);
        PORTAL_CORE_ASSERT(property.value.size / property.elements_number % sizeof(typename T::value_type) == 0, "Property {} size mismatch", name);

        auto array_length = property.elements_number;
        auto* data = static_cast<typename T::value_type*>(property.value.data);
        return T(data, data + array_length);
    }

    template <serialization::String T>
    T get_property(const std::string& name)
    {
        PORTAL_CORE_ASSERT(properties.contains(name), "Property {} not found", name);


        const auto& property = properties.at(name);

        size_t string_length;
        if (property.container_type == serialization::PropertyContainerType::null_term_string)
            string_length = property.elements_number - 1;
        else if (property.container_type == serialization::PropertyContainerType::string)
            string_length = property.elements_number;
        else
        {
            LOG_CORE_ERROR_TAG("Serialization", "Property {} container type mismatch", name);
            string_length = 0;
        }

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        return T(data, string_length);
    }

    template <serialization::GlmVec1 T>
    T get_property(const std::string& name)
    {
        PORTAL_CORE_ASSERT(properties.contains(name), "Property {} not found", name);

        const auto& property = properties.at(name);

        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::vec1, "Property {} container type mismatch", name);

        return T(*static_cast<typename T::value_type*>(property.value.data));
    }

    template <serialization::GlmVec2 T>
    T get_property(const std::string& name)
    {
        PORTAL_CORE_ASSERT(properties.contains(name), "Property {} not found", name);

        const auto& property = properties.at(name);

        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::vec2, "Property {} container type mismatch", name);

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        return T(data[0], data[1]);
    }

    template <serialization::GlmVec3 T>
    T get_property(const std::string& name)
    {
        PORTAL_CORE_ASSERT(properties.contains(name), "Property {} not found", name);

        const auto& property = properties.at(name);

        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::vec3, "Property {} container type mismatch", name);

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        return T(data[0], data[1], data[2]);
    }

    template <serialization::GlmVec4 T>
    T get_property(const std::string& name)
    {
        PORTAL_CORE_ASSERT(properties.contains(name), "Property {} not found", name);

        const auto& property = properties.at(name);

        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::vec4, "Property {} container type mismatch", name);

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        return T(data[0], data[1], data[2], data[3]);
    }

protected:
    std::map<std::string, serialization::Property> properties;
};


class OrderedDeserializer : public Deserializer
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

// TODO: rename functions as its a bit confusing that the `serialize` only adds itself to the serializer while the
// `deserialize` calls the `d.deserialize()` function on the deserializer in order to create itself.
template <typename T>
concept Serializable = requires(T t, Archiver& s) {
    { t.serialize(s) } -> std::same_as<void>;
};

template <typename T>
concept PackedSerialzable = requires(T t, OrderedSerializer& s) {
    { t.serialize(s) } -> std::same_as<void>;
};

template <typename T>
concept Deserializable = requires(T t, Deserializer& d) {
    { T::deserialize(d) } -> std::same_as<T>;
};

} // namespace portal

template <portal::Serializable T>
portal::Archiver& operator<<(portal::Archiver& s, T& t)
{
    t.serialize(s);
    return s;
}

template <portal::PackedSerialzable T>
portal::OrderedSerializer& operator<<(portal::OrderedSerializer& s, T& t)
{
    t.serialize(s);
    return s;
}

template <portal::serialization::PropertyConcept T>
portal::OrderedSerializer& operator<<(portal::OrderedSerializer& s, T& t)
{
    s.add_property(t);
    return s;
}
