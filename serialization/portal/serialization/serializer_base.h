//
// Created by Jonatan Nevo on 18/02/2025.
//

#pragma once

#include <concepts>
#include <limits>

#include "property.h"

namespace portal::serialization
{

template <typename T>
consteval auto get_property_type()
{
    if constexpr (std::integral<T>)
    {
        if constexpr (sizeof(T) == 1)
            return PropertyType::integer8;
        else if constexpr (sizeof(T) == 2)
            return PropertyType::integer16;
        else if constexpr (sizeof(T) == 4)
            return PropertyType::integer32;
        else if constexpr (sizeof(T) == 8)
            return PropertyType::integer64;
        else if constexpr (sizeof(T) == 16)
            return PropertyType::integer128;
        else
            return PropertyType::invalid;
    }
    else if constexpr (std::floating_point<T>)
    {
        if constexpr (sizeof(T) == 4)
            return PropertyType::floating32;
        else if constexpr (sizeof(T) == 8)
            return PropertyType::floating64;
        else
            return PropertyType::invalid;
    }
    else
    {
        return PropertyType::binary;
    }
}

class Serializer
{
public:
    virtual ~Serializer() = default;
    virtual void serialize() = 0;

    template <typename T>
        requires std::integral<T> || std::floating_point<T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {Buffer{&t, sizeof(T)}, get_property_type<T>(), PropertyContainerType::scalar, 1};
    }

    template <Vector T>
    void add_property(const std::string& name, T& t)
    {
        if (t.size() > static_cast<size_t>((std::numeric_limits<uint16_t>::max)()))
            throw std::runtime_error("Vector too long, cannot serialize");

        properties[name] = {
            Buffer{t.data(), t.size() * sizeof(typename T::value_type)},
            get_property_type<typename T::value_type>(),
            PropertyContainerType::array,
            static_cast<uint16_t>(t.size())
        };
    }

    template <String T>
    void add_property(const std::string& name, T& t)
    {
        if (t.size() >= static_cast<size_t>((std::numeric_limits<uint16_t>::max)()))
            throw std::runtime_error("String too long, cannot serialize");

        properties[name] = {
            Buffer{t.data(), (t.size() * sizeof(typename T::value_type)) + 1},
            PropertyType::character,
            PropertyContainerType::null_term_string,
            static_cast<uint16_t>(t.size() + 1)
        };
    }

    template <GlmVec1 T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t.x, sizeof(typename T::value_type)},
            get_property_type<typename T::value_type>(),
            PropertyContainerType::vector,
            1
        };
    }

    template <GlmVec2 T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t.x, 2 * sizeof(typename T::value_type)},
            get_property_type<typename T::value_type>(),
            PropertyContainerType::vector,
            2
        };
    }

    template <GlmVec3 T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t.x, 3 * sizeof(typename T::value_type)},
            get_property_type<typename T::value_type>(),
            PropertyContainerType::vector,
            3
        };
    }

    template <GlmVec4 T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t.x, 4 * sizeof(typename T::value_type)},
            get_property_type<typename T::value_type>(),
            PropertyContainerType::vector,
            4
        };
    }

protected:
    std::map<std::string, Property> properties;
};

class OrderedSerializer : protected Serializer
{
public:
    void serialize() override = 0;

    template <typename T>
    void add_property(T& t)
    {
        Serializer::add_property(std::to_string(counter++), t);
    };

protected:
    size_t counter = 0;
};

} // namespace portal::serialization
