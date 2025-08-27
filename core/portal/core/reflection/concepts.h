//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <concepts>
#include <vector>
#include <string>
#include <portal/core/glm.h>

#include "portal/core/reflection/property.h"

namespace portal::reflection
{
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
    requires (std::same_as<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>> || std::same_as<
        T, std::basic_string_view<typename T::value_type, typename T::traits_type>>);
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

template<typename T>
concept IsVec = GlmVec1<T> || GlmVec2<T> || GlmVec3<T> || GlmVec4<T>;

template <typename T>
concept GlmMat2 = requires(T t) {
    requires std::same_as<T, glm::mat2x2> || std::same_as<T, glm::dmat2x2> || std::same_as<T, glm::imat2x2>;
};

template <typename T>
concept GlmMat3 = requires(T t) {
    requires std::same_as<T, glm::mat3x3> || std::same_as<T, glm::dmat3x3> || std::same_as<T, glm::imat3x3>;
};

template <typename T>
concept GlmMat4 = requires(T t) {
    requires std::same_as<T, glm::mat4x4> || std::same_as<T, glm::dmat4x4> || std::same_as<T, glm::imat4x4>;
};

template <typename T>
concept IsMatrix = GlmMat2<T> || GlmMat3<T> || GlmMat4<T>;

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
consteval reflection::PropertyType get_property_type()
{
    if constexpr (std::integral<T>)
    {
        return reflection::get_integer_type(sizeof(T));
    }
    else if constexpr (std::floating_point<T>)
    {
        return reflection::get_float_type(sizeof(T));
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        return reflection::PropertyType::boolean;
    }
    else if constexpr (std::is_same_v<T, char>)
    {
        return reflection::PropertyType::character;
    }
    else if constexpr (String<T>)
    {
        return reflection::PropertyType::character;
    }
    else
    {
        return reflection::PropertyType::binary;
    }
}
}
