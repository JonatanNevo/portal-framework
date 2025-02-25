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

    template <typename T> requires std::integral<T> || std::floating_point<T>
    void add_value(T& t)
    {
        add_property(
            serialization::Property{
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
        add_property(
            serialization::Property{
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
        add_property(
            serialization::Property{
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
        add_property(
            serialization::Property{
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
        add_property(
            serialization::Property{
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
        add_property(
            serialization::Property{
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
        add_property(
            serialization::Property{
                Buffer{&t.x, 4 * sizeof(typename T::value_type)},
                serialization::get_property_type<typename T::value_type>(),
                serialization::PropertyContainerType::vec4,
                4
            }
        );
    }

protected:
    virtual void add_property(serialization::Property property) = 0;
};

class Deserializer
{
public:
    virtual ~Deserializer() = default;

    template <typename T> requires std::integral<T> || std::floating_point<T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::scalar, "Property container type mismatch");
        PORTAL_CORE_ASSERT(property.type == serialization::get_property_type<T>(), "Property type mismatch");
        PORTAL_CORE_ASSERT(property.value.size == sizeof(T), "Value size mismatch, expected: {} got {}", sizeof(T), property.value.size);

        t = *static_cast<T*>(property.value.data);
    }

    template <serialization::Vector T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::array, "Property container type mismatch");
        PORTAL_CORE_ASSERT(property.type == serialization::get_property_type<typename T::value_type>(), "Property type mismatch");

        auto array_length = property.elements_number;
        auto* data = static_cast<typename T::value_type*>(property.value.data);
        t = T(data, data + array_length);
    }

    template <serialization::String T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_CORE_ASSERT(property.type == serialization::PropertyType::character, "Property type mismatch");

        size_t string_length;
        if (property.container_type == serialization::PropertyContainerType::null_term_string)
            string_length = property.elements_number - 1;
        else if (property.container_type == serialization::PropertyContainerType::string)
            string_length = property.elements_number;

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        t = T(data, string_length);
    }

    template <serialization::GlmVec1 T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_CORE_ASSERT(property.type == serialization::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::vec1, "Property container type mismatch");

        t = T(*static_cast<typename T::value_type*>(property.value.data));
    }

    template <serialization::GlmVec2 T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_CORE_ASSERT(property.type == serialization::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::vec2, "Property container type mismatch");

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        t = T(data[0], data[1]);
    }

    template <serialization::GlmVec3 T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_CORE_ASSERT(property.type == serialization::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::vec3, "Property container type mismatch");

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        t = T(data[0], data[1], data[2]);
    }

    template <serialization::GlmVec4 T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_CORE_ASSERT(property.type == serialization::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::vec4, "Property container type mismatch");

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        t = T(data[0], data[1], data[2], data[3]);
    }

    template <typename T>
    T get_value()
    {
        T t;
        get_value<T>(t);
        return t;
    }

protected:
    virtual serialization::Property get_property() = 0;
};

template <typename T>
concept Serializable = requires(T t, Serializer& s) {
    { t.serialize(s) } -> std::same_as<void>;
};

template <typename T>
concept Deserializable = requires(T t, Deserializer& d) {
    { T::deserialize(d) } -> std::same_as<T>;
};
} // namespace portal

template <portal::Serializable T>
portal::Serializer& operator<<(portal::Serializer& s, T& t)
{
    t.serialize(s);
    return s;
}

template <portal::serialization::PropertyConcept T>
portal::Serializer& operator<<(portal::Serializer& s, T& t)
{
    s.add_value<T>(t);
    return s;
}

template <typename T> requires std::is_enum_v<T>
portal::Serializer& operator<<(portal::Serializer& s, const T& value)
{
    using underlying_t = std::underlying_type_t<T>;
    s.add_value<underlying_t>(static_cast<underlying_t>(value));
    return s;
}
