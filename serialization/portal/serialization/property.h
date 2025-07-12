//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <concepts>
#include <portal/core/buffer.h>

namespace portal::serialize
{
enum class PropertyType : uint8_t
{
    binary           = 0,
    integer8         = 1,
    integer16        = 2,
    integer32        = 3,
    integer64        = 4,
    integer128       = 5,
    floating32       = 6,
    floating64       = 7,
    character        = 8,
    boolean          = 9,
    object           = 10,
    null_term_string = 11,
    string           = 12,
    invalid          = 255
};

enum class PropertyContainerType : uint8_t
{
    invalid             = 0,
    scalar              = 1,
    array               = 2,
    string              = 3,
    null_term_string    = 4,
    vec1                = 5,
    __vector_type_start = vec1,
    vec2                = 6,
    vec3                = 7,
    vec4                = 8,
    object              = 9
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
    PropertyContainerType container_type = PropertyContainerType::invalid;
    size_t elements_number = 0;
};

constexpr PropertyType get_integer_type(const size_t size)
{
    if (size == 1)
        return PropertyType::integer8;
    if (size == 2)
        return PropertyType::integer16;
    if (size == 4)
        return PropertyType::integer32;
    if (size == 8)
        return PropertyType::integer64;
    if (size == 16)
        return PropertyType::integer128;
    return PropertyType::invalid;
}

constexpr PropertyType get_float_type(const size_t size)
{
    if (size == 4)
        return PropertyType::floating32;
    if (size == 8)
        return PropertyType::floating64;
    return PropertyType::invalid;
}


template <typename T>
concept Vector = requires(T t) {
    typename T::value_type;
    typename T::allocator_type;
    requires std::same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>;
};

template <typename T>
concept String = requires(T t) {
    typename T::value_type;
    typename T::traits_type;
    typename T::allocator_type;
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
    typename T::iterator;
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

template <typename T>
consteval PropertyType get_property_type()
{
    if constexpr (std::integral<T>)
    {
        return get_integer_type(sizeof(T));
    }
    else if constexpr (std::floating_point<T>)
    {
        return get_float_type(sizeof(T));
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        return PropertyType::boolean;
    }
    else if constexpr (std::is_same_v<T, char>)
    {
        return PropertyType::character;
    }
    else if constexpr (String<T>)
    {
        return PropertyType::character;
    }
    else
    {
        return PropertyType::binary;
    }
}
} // namespace portal::serialization
