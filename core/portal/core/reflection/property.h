//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/buffer.h>

namespace portal::reflection
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
} // namespace portal::serialization
