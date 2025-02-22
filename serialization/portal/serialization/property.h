//
// Created by Jonatan Nevo on 18/02/2025.
//

#pragma once

#include <concepts>
#include <portal/core/buffer.h>

namespace portal::serialization
{

enum class PropertyType : uint8_t
{
    binary = 0,
    integer = 1,
    floating = 2,
    character = 4,
    invalid = 255
};

enum class PropertyContainerType : uint8_t
{
    scalar = 0,
    array = 1,
    vector = 2,
};

struct Property
{
    Buffer value;
    PropertyType type{};
    PropertyContainerType container_type = PropertyContainerType::scalar;
};

template <typename T>
concept Vector = requires(T t) { requires std::same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>; };

template <typename T>
concept String =
    requires(T t) { requires std::same_as<T, std::basic_string<typename T::value_type, typename T::traits_type, typename T::allocator_type>>; };

template <typename T>
concept GlmVec1 = requires(T t) { requires std::same_as<T, glm::vec1> || std::same_as<T, glm::dvec1> || std::same_as<T, glm::ivec1>; };

template <typename T>
concept GlmVec2 = requires(T t) { requires std::same_as<T, glm::vec2> || std::same_as<T, glm::dvec2> || std::same_as<T, glm::ivec2>; };

template <typename T>
concept GlmVec3 = requires(T t) { requires std::same_as<T, glm::vec3> || std::same_as<T, glm::dvec3> || std::same_as<T, glm::ivec3>; };

template <typename T>
concept GlmVec4 = requires(T t) { requires std::same_as<T, glm::vec4> || std::same_as<T, glm::dvec4> || std::same_as<T, glm::ivec4>; };

} // namespace portal::serialization
