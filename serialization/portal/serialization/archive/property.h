//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <concepts>
#include <portal/core/buffer.h>

namespace portal::archiving
{
enum class PropertyType : uint8_t
{
    binary     = 0,
    integer8   = 1,
    integer16  = 2,
    integer32  = 3,
    integer64  = 4,
    integer128 = 5,
    floating32 = 6,
    floating64 = 7,
    character  = 8,
    object     = 9,
    boolean    = 10,
    invalid    = 255
};

enum class PropertyContainerType : uint8_t
{
    scalar              = 0,
    array               = 1,
    string              = 2,
    null_term_string    = 3,
    vec1                = 4,
    __vector_type_start = vec1,
    vec2                = 5,
    vec3                = 6,
    vec4                = 7,
    object              = 8
};

constexpr bool is_vector_type(const PropertyContainerType type)
{
    return type == PropertyContainerType::vec1 || type == PropertyContainerType::vec2 ||
        type == PropertyContainerType::vec3 || type == PropertyContainerType::vec4;
}

struct Property
{
    Buffer value{};
    PropertyType type = PropertyType::invalid;
    PropertyContainerType container_type = PropertyContainerType::scalar;
    size_t elements_number = 0;
};

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


template <typename T>
concept Vector = requires(T t) { requires std::same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>; };

template <typename T>
concept String = requires(T t) {
    requires std::same_as<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>;
};

template <typename T>
concept GlmVec1 = requires(T t) {
    requires std::same_as<T, glm::vec1> || std::same_as<T, glm::dvec1> || std::same_as<T, glm::ivec1>;
};

template <typename T>
concept GlmVec2 = requires(T t) {
    requires std::same_as<T, glm::vec2> || std::same_as<T, glm::dvec2> || std::same_as<T, glm::ivec2>;
};

template <typename T>
concept GlmVec3 = requires(T t) {
    requires std::same_as<T, glm::vec3> || std::same_as<T, glm::dvec3> || std::same_as<T, glm::ivec3>;
};

template <typename T>
concept GlmVec4 = requires(T t) {
    requires std::same_as<T, glm::vec4> || std::same_as<T, glm::dvec4> || std::same_as<T, glm::ivec4>;
};

template <typename T>
concept Map = requires(T t) {
    typename T::key_type;
    typename T::mapped_type;
    typename T::value_type;
    { t.size() } -> std::same_as<size_t>;
    { t.begin() } -> std::same_as<typename T::iterator>;
    { t.end() } -> std::same_as<typename T::iterator>;
    { t.clear() } -> std::same_as<void>;
    { t.insert_or_assign(std::declval<typename T::key_type>(), std::declval<typename T::mapped_type>()) };
};

template <typename T>
concept PropertyConcept = requires(T t) {
    requires Vector<T> || String<T> || GlmVec1<T> || GlmVec2<T> || GlmVec3<T> || GlmVec4<T> || Map<T> || std::integral<T> ||
    std::floating_point<T>;
};
} // namespace portal::serialization
