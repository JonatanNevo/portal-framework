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

    template <typename T>
        requires std::integral<T> || std::floating_point<T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t, sizeof(T)},
            serialization::get_property_type<T>(),
            serialization::PropertyContainerType::scalar,
            1};
    }

    template <serialization::Vector T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{t.data(), t.size() * sizeof(typename T::value_type)},
            serialization::get_property_type<typename T::value_type>(),
            serialization::PropertyContainerType::array,
            t.size()};
    }

    template <serialization::String T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{t.data(), (t.size() * sizeof(typename T::value_type)) + 1},
            serialization::PropertyType::character,
            serialization::PropertyContainerType::null_term_string,
            t.size() + 1};
    }

    template <serialization::GlmVec1 T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t.x, sizeof(typename T::value_type)},
            serialization::get_property_type<typename T::value_type>(),
            serialization::PropertyContainerType::vec1,
            1};
    }

    template <serialization::GlmVec2 T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t.x, 2 * sizeof(typename T::value_type)},
            serialization::get_property_type<typename T::value_type>(),
            serialization::PropertyContainerType::vec2,
            2};
    }

    template <serialization::GlmVec3 T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t.x, 3 * sizeof(typename T::value_type)},
            serialization::get_property_type<typename T::value_type>(),
            serialization::PropertyContainerType::vec3,
            3};
    }

    template <serialization::GlmVec4 T>
    void add_property(const std::string& name, T& t)
    {
        properties[name] = {
            Buffer{&t.x, 4 * sizeof(typename T::value_type)},
            serialization::get_property_type<typename T::value_type>(),
            serialization::PropertyContainerType::vec4,
            4};
    }

protected:
    std::unordered_map<std::string, serialization::Property> properties;
};

}