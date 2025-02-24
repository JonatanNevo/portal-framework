//
// Created by Jonatan Nevo on 24/02/2025.
//

#pragma once
#include <concepts>
#include <string>

#include "portal/core/buffer.h"
#include "portal/serialization/property.h"

namespace portal
{
class Archiver
{
public:
    virtual ~Archiver() = default;
    virtual void archive() = 0;

    template <typename T> requires std::integral<T> || std::floating_point<T>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {
                Buffer{&t, sizeof(T)},
                serialization::get_property_type<T>(),
                serialization::PropertyContainerType::scalar,
                1
            }
        );
    }

    template <serialization::Vector T>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {
                Buffer{t.data(), t.size() * sizeof(typename T::value_type)},
                serialization::get_property_type<typename T::value_type>(),
                serialization::PropertyContainerType::array,
                t.size()
            }
        );
    }

    template <serialization::String T>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {
                Buffer{t.data(), (t.size() * sizeof(typename T::value_type)) + 1},
                serialization::PropertyType::character,
                serialization::PropertyContainerType::null_term_string,
                t.size() + 1
            }
        );
    }

    template <serialization::GlmVec1 T>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {
                Buffer{&t.x, sizeof(typename T::value_type)},
                serialization::get_property_type<typename T::value_type>(),
                serialization::PropertyContainerType::vec1,
                1
            }
        );
    }

    template <serialization::GlmVec2 T>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {
                Buffer{&t.x, 2 * sizeof(typename T::value_type)},
                serialization::get_property_type<typename T::value_type>(),
                serialization::PropertyContainerType::vec2,
                2
            }
        );
    }

    template <serialization::GlmVec3 T>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {
                Buffer{&t.x, 3 * sizeof(typename T::value_type)},
                serialization::get_property_type<typename T::value_type>(),
                serialization::PropertyContainerType::vec3,
                3
            }
        );
    }

    template <serialization::GlmVec4 T>
    void add_property(const std::string& name, T& t)
    {
        add_property(
            name,
            {
                Buffer{&t.x, 4 * sizeof(typename T::value_type)},
                serialization::get_property_type<typename T::value_type>(),
                serialization::PropertyContainerType::vec4,
                4
            }
        );
    }

protected:
    virtual void add_property(const std::string& name, const serialization::Property& property) = 0;
};


class Dearchiver
{
public:
    virtual ~Dearchiver() = default;
    virtual void dearchive() = 0;

    template <typename T> requires std::integral<T> || std::floating_point<T>
    bool get_property(const std::string& name, T& out)
    {
        serialization::Property property;
        if (!get_property(name, property))
            return false;

        PORTAL_CORE_ASSERT(property.type == serialization::get_property_type<T>(), "Property {} type mismatch", name);

        out = *static_cast<T*>(property.value.data);
        return true;
    }

    template <serialization::Vector T>
    bool get_property(const std::string& name, T& out)
    {
        serialization::Property property;
        if (!get_property(name, property))
            return false;

        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::array, "Property {} container type mismatch", name);
        PORTAL_CORE_ASSERT(property.value.size / property.elements_number % sizeof(typename T::value_type) == 0, "Property {} size mismatch", name);

        auto array_length = property.elements_number;
        auto* data = static_cast<typename T::value_type*>(property.value.data);
        out = T(data, data + array_length);
        return true;
    }

    template <serialization::String T>
    bool get_property(const std::string& name, T& out)
    {
        serialization::Property property;
        if (!get_property(name, property))
            return false;

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
        out = T(data, string_length);
        return true;
    }

    template <serialization::GlmVec1 T>
    bool get_property(const std::string& name, T& out)
    {
        serialization::Property property;
        if (!get_property(name, property))
            return false;

        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::vec1, "Property {} container type mismatch", name);

        out = T(*static_cast<typename T::value_type*>(property.value.data));
        return true;
    }

    template <serialization::GlmVec2 T>
    bool get_property(const std::string& name, T& out)
    {
        serialization::Property property;
        if (!get_property(name, property))
            return false;

        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::vec2, "Property {} container type mismatch", name);

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        out = T(data[0], data[1]);
        return true;
    }

    template <serialization::GlmVec3 T>
    bool get_property(const std::string& name, T& out)
    {
        serialization::Property property;
        if (!get_property(name, property))
            return false;

        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::vec3, "Property {} container type mismatch", name);

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        out = T(data[0], data[1], data[2]);
        return true;
    }

    template <serialization::GlmVec4 T>
    bool get_property(const std::string& name, T& out)
    {
        serialization::Property property;
        if (!get_property(name, property))
            return false;

        PORTAL_CORE_ASSERT(property.container_type == serialization::PropertyContainerType::vec4, "Property {} container type mismatch", name);

        const auto* data = static_cast<const typename T::value_type*>(property.value.data);
        out = T(data[0], data[1], data[2], data[3]);
        return true;
    }

protected:
    virtual bool get_property(const std::string& name, serialization::Property& out) = 0;
};


template <typename T>
concept Archiveable = requires(T t, Archiver& s) {
    { t.archive(s) } -> std::same_as<void>;
};

template <typename T>
concept Dearchiveable = requires(T t, Dearchiver& d) {
    { T::dearchive(d) } -> std::same_as<T>;
};
} // namespace portal

