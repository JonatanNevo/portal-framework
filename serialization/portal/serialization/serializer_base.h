//
// Created by Jonatan Nevo on 18/02/2025.
//

#pragma once

#include <concepts>

#include "property.h"

namespace portal::serialization
{

template <typename T>
consteval auto get_property_type()
{
    if constexpr (std::integral<T>)
    {
        return PropertyType::integer;
    }
    else if constexpr (std::floating_point<T>)
    {
        return PropertyType::floating;
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

    template <std::integral T>
    void add_property(const std::string& name, T& t)
    {
         properties[name]= {Buffer{&t, sizeof(T)}, PropertyType::integer};
    }

    template <std::floating_point T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {Buffer{&t, sizeof(T)}, PropertyType::floating};
    }

    template <Vector T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{t.data(), t.size() * sizeof(typename T::value_type)},
            serialization::get_property_type<typename T::value_type>(),
            PropertyContainerType::array};
    }

    template <String T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{t.data(), (t.size() * sizeof(typename T::value_type)) + 1},
            PropertyType::character,
            PropertyContainerType::array};
    }

    template <GlmVec1 T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t.x, sizeof(typename T::value_type)},
            serialization::get_property_type<typename T::value_type>(),
            PropertyContainerType::vector};
    }

    template <GlmVec2 T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t.x, 2 * sizeof(typename T::value_type)},
            serialization::get_property_type<typename T::value_type>(),
            PropertyContainerType::vector};
    }

    template <GlmVec3 T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t.x, 3 * sizeof(typename T::value_type)},
            serialization::get_property_type<typename T::value_type>(),
            PropertyContainerType::vector};
    }

    template <GlmVec4 T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t.x, 4 * sizeof(typename T::value_type)},
            serialization::get_property_type<typename T::value_type>(),
            PropertyContainerType::vector};
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
