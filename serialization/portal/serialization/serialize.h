//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/log.h>
#include <portal/core/reflection/property.h>

#include "portal/serialization/concepts.h"

namespace portal
{
class Serializer;
class Deserializer;

template <typename T>
concept Serializable = requires(const T t, Serializer& s) {
    { t.serialize(s) } -> std::same_as<void>;
};

template <typename T>
concept Deserializable = requires(T t, Deserializer& d) {
    { T::deserialize(d) } -> std::same_as<T>;
};

class Serializer
{
public:
    virtual ~Serializer() = default;

    template <typename T> requires std::integral<std::remove_const_t<T>> || std::floating_point<std::remove_const_t<T>>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(&t)), sizeof(T)},
                serialize::get_property_type<std::remove_const_t<T>>(),
                reflection::PropertyContainerType::scalar,
                1
            }
        );
    }

    template <typename T> requires std::is_same_v<T, uint128_t>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(&t)), sizeof(T)},
                reflection::PropertyType::integer128,
                reflection::PropertyContainerType::scalar,
                1
            }
        );
    }


    template <serialize::Vector T> requires (!Serializable<typename T::value_type>)
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(t.data())), t.size() * sizeof(typename T::value_type)},
                serialize::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::array,
                t.size()
            }
        );
    }

    template <serialize::String T>
    void add_value(const T& t)
    {
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(t.data())), (t.size() * sizeof(typename T::value_type)) + 1},
                reflection::PropertyType::character,
                reflection::PropertyContainerType::null_term_string,
                t.size() + 1
            }
        );
    }

    template <serialize::GlmVec1 T>
    void add_value(T& t)
    {
        add_property(
            reflection::Property{
                Buffer{&t.x, sizeof(typename T::value_type)},
                serialize::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                1
            }
        );
    }

    template <serialize::GlmVec2 T>
    void add_value(T& t)
    {
        add_property(
            reflection::Property{
                Buffer{&t.x, 2 * sizeof(typename T::value_type)},
                serialize::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                2
            }
        );
    }

    template <serialize::GlmVec3 T>
    void add_value(T& t)
    {
        add_property(
            reflection::Property{
                Buffer{&t.x, 3 * sizeof(typename T::value_type)},
                serialize::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                3
            }
        );
    }

    template <serialize::GlmVec4 T>
    void add_value(T& t)
    {
        add_property(
            reflection::Property{
                Buffer{&t.x, 4 * sizeof(typename T::value_type)},
                serialize::get_property_type<typename T::value_type>(),
                reflection::PropertyContainerType::vector,
                4
            }
        );
    }

    template <serialize::Map T>
    void add_value(const T& t)
    {
        const size_t size = t.size();
        add_value(size);

        for (const auto& [key, value] : t)
        {
            using KeyType = std::remove_const_t<typename T::key_type>;
            using ValueType = std::remove_const_t<typename T::mapped_type>;

            add_value<KeyType>(key);
            add_value<ValueType>(value);
        }
    }

    template <serialize::Vector T> requires Serializable<typename T::value_type>
    void add_value(const T& t)
    {
        const size_t size = t.size();
        add_value(size);

        for (const auto& value : t)
        {
            using ValueType = std::remove_const_t<typename T::value_type>;
            add_value<ValueType>(value);
        }
    }

    template <typename T> requires std::is_enum_v<T>
    void add_value(const T& t)
    {
        add_value<std::underlying_type_t<T>>(static_cast<std::underlying_type_t<T>>(t));
    }

    template <Serializable T>
    void add_value(const T& t)
    {
        t.serialize(*this);
    }

    void add_value(const char* t)
    {
        const size_t length = strlen(t);
        add_property(
            reflection::Property{
                Buffer{const_cast<void*>(static_cast<const void*>(t)), (length * sizeof(char)) + 1},
                reflection::PropertyType::character,
                reflection::PropertyContainerType::null_term_string,
                length + 1
            }
        );
    }

    void add_value(void* t, const size_t length)
    {
        add_property(
            reflection::Property{Buffer{t, length}, reflection::PropertyType::binary, reflection::PropertyContainerType::string, length}
        );
    }

protected:
    virtual void add_property(reflection::Property property) = 0;
};

class Deserializer
{
public:
    virtual ~Deserializer() = default;

    template <typename T> requires std::integral<T> || std::floating_point<T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::scalar, "Property container type mismatch");
        PORTAL_ASSERT(property.type == serialize::get_property_type<T>(), "Property type mismatch");
        PORTAL_ASSERT(property.value.size == sizeof(T), "Value size mismatch, expected: {} got {}", sizeof(T), property.value.size);

        t = *static_cast<T*>(property.value.data);
    }

    template <typename T> requires std::is_same_v<T, uint128_t>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::scalar, "Property container type mismatch");
        PORTAL_ASSERT(property.type == reflection::PropertyType::integer128, "Property type mismatch");
        PORTAL_ASSERT(property.value.size == sizeof(T), "Value size mismatch, expected: {} got {}", sizeof(T), property.value.size);

        t = *static_cast<T*>(property.value.data);
    }

    template <serialize::Vector T> requires (!Serializable<typename T::value_type>)
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::array, "Property container type mismatch");
        PORTAL_ASSERT(property.type == serialize::get_property_type<typename T::value_type>(), "Property type mismatch");

        auto array_length = property.elements_number;
        auto* data = static_cast<typename T::value_type*>(property.value.data);
        t = T(data, data + array_length);
    }

    template <serialize::String T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_ASSERT(property.type == reflection::PropertyType::character, "Property type mismatch");

        size_t string_length;
        if (property.container_type == reflection::PropertyContainerType::null_term_string)
            string_length = property.elements_number - 1;
        else if (property.container_type == reflection::PropertyContainerType::string)
            string_length = property.elements_number;

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        t = T(data, string_length);
    }

    template <serialize::GlmVec1 T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_ASSERT(property.type == serialize::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property container type mismatch");
        PORTAL_ASSERT(property.elements_number == 1, "Property elements number mismatch");

        t = T(*static_cast<typename T::value_type*>(property.value.data));
    }

    template <serialize::GlmVec2 T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_ASSERT(property.type == serialize::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property container type mismatch");
        PORTAL_ASSERT(property.elements_number == 2, "Property elements number mismatch");

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        t = T(data[0], data[1]);
    }

    template <serialize::GlmVec3 T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_ASSERT(property.type == serialize::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property container type mismatch");
        PORTAL_ASSERT(property.elements_number == 3, "Property elements number mismatch");

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        t = T(data[0], data[1], data[2]);
    }

    template <serialize::GlmVec4 T>
    void get_value(T& t)
    {
        const auto property = get_property();

        PORTAL_ASSERT(property.type == serialize::get_property_type<typename T::value_type>(), "Property type mismatch");
        PORTAL_ASSERT(property.container_type == reflection::PropertyContainerType::vector, "Property container type mismatch");
        PORTAL_ASSERT(property.elements_number == 4, "Property elements number mismatch");

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        t = T(data[0], data[1], data[2], data[3]);
    }

    template <serialize::Map T>
    void get_value(T& t)
    {
        size_t size;
        get_value<size_t>(size);

        t.clear();
        if constexpr (requires { t.reserve(size); })
        {
            t.reserve(size);
        }

        for (size_t i = 0; i < size; ++i)
        {
            typename T::key_type key;
            typename T::mapped_type value;
            get_value(key);
            get_value(value);
            t.insert_or_assign(std::move(key), std::move(value));
        }
    }

    template <serialize::Vector T> requires Serializable<typename T::value_type>
    void get_value(T& t)
    {
        size_t size;
        get_value<size_t>(size);

        if constexpr (requires { t.reserve(size); })
        {
            t.reserve(size);
        }

        for (size_t i = 0; i < size; ++i)
        {
            typename T::value_type value;
            get_value<typename T::value_type>(value);
            t.push_back(std::move(value));
        }
    }

    template <typename T> requires std::is_enum_v<T>
    T get_value()
    {
        std::underlying_type_t<T> underlying;
        get_value(underlying);
        return static_cast<T>(underlying);
    }

    template <Deserializable T>
    void get_value(T& t)
    {
        t = T::deserialize(*this);
    }

    template <typename T>
    T get_value()
    {
        T t;
        get_value<T>(t);
        return t;
    }

    void get_value(char*& t, const size_t length)
    {
        const auto property = get_property();

        PORTAL_ASSERT(property.type == reflection::PropertyType::character, "Property type mismatch");

        PORTAL_ASSERT(property.elements_number == length, "Value size mismatch, expected: {} got {}", length, property.elements_number);
        memcpy(t, property.value.data, (std::min)(length, property.elements_number));
    }

protected:
    virtual reflection::Property get_property() = 0;
};
} // namespace portal

template <portal::Serializable T>
portal::Serializer& operator<<(portal::Serializer& s, const T& t)
{
    t.serialize(s);
    return s;
}

template <portal::serialize::PropertyConcept T>
portal::Serializer& operator<<(portal::Serializer& s, const T& t)
{
    T copy = t;
    s.add_value<T>(copy);
    return s;
}

template <portal::serialize::PropertyConcept T>
portal::Serializer& operator<<(portal::Serializer& s, T& t)
{
    s.add_value<T>(t);
    return s;
}

template <typename T> requires std::is_enum_v<T>
portal::Serializer& operator<<(portal::Serializer& s, const T& value)
{
    return s << static_cast<std::underlying_type_t<T>>(value);
}

inline portal::Serializer& operator<<(portal::Serializer& s, const char* str)
{
    s.add_value(str);
    return s;
}

template <typename T>
portal::Serializer* operator<<(portal::Serializer* s, const T& t)
{
    return &(*s << t);
}


template <portal::Deserializable T>
portal::Deserializer& operator>>(portal::Deserializer& d, T& t)
{
    t = T::deserialize(d);
    return d;
}

template <portal::serialize::PropertyConcept T>
portal::Deserializer& operator>>(portal::Deserializer& d, T& t)
{
    d.get_value<T>(t);
    return d;
}

template <typename T> requires std::is_enum_v<T>
portal::Deserializer& operator>>(portal::Deserializer& d, T& t)
{
    std::underlying_type_t<T> underlying;
    d >> underlying;
    t = static_cast<T>(underlying);
    return d;
}
