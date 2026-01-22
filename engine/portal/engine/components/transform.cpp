//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "transform.h"
#include <glm/gtx/matrix_decompose.hpp>

auto wrap_to_pi(const glm::vec3 v)
{
    return glm::mod(v + glm::pi<float>(), 2.0f * glm::pi<float>()) - glm::pi<float>();
}

namespace portal
{
TransformComponent::TransformComponent(const glm::vec3& translation) : translation(translation) {}

TransformComponent::TransformComponent(const glm::mat4& transform)
{
    set_matrix(transform);
}

TransformComponent::TransformComponent(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
    : translation(translation), rotation(rotation), scale(scale), rotation_euler(glm::eulerAngles(rotation))
{}

void TransformComponent::set_matrix(const glm::mat4& matrix)
{
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(matrix, scale, rotation, translation, skew, perspective);
    rotation_euler = glm::eulerAngles(rotation);
}

void TransformComponent::set_translation(const glm::vec3& new_translation)
{
    translation = new_translation;
}

void TransformComponent::set_rotation(const glm::quat& new_rotation)
{
    glm::vec3 original_euler = rotation_euler;

    rotation = new_rotation;
    rotation_euler = glm::eulerAngles(rotation);

    // A given quat can be represented by many Euler angles (technically infinitely many),
    // and glm::eulerAngles() can only give us one of them which may or may not be the one we want.
    // Here we have a look at some likely alternatives and pick the one that is closest to the original Euler angles.
    // This is an attempt to avoid sudden 180deg flips in the Euler angles when we SetRotation(quat).
    glm::vec3 alternate_1 = {rotation_euler.x - glm::pi<float>(), glm::pi<float>() - rotation_euler.y, rotation_euler.z - glm::pi<float>()};
    glm::vec3 alternate_2 = {rotation_euler.x + glm::pi<float>(), glm::pi<float>() - rotation_euler.y, rotation_euler.z - glm::pi<float>()};
    glm::vec3 alternate_3 = {rotation_euler.x + glm::pi<float>(), glm::pi<float>() - rotation_euler.y, rotation_euler.z + glm::pi<float>()};
    glm::vec3 alternate_4 = {rotation_euler.x - glm::pi<float>(), glm::pi<float>() - rotation_euler.y, rotation_euler.z + glm::pi<float>()};

    // We pick the alternative that is closest to the original value.

    float distance_0 = glm::length2(wrap_to_pi(rotation_euler - original_euler));
    float distance_1 = glm::length2(wrap_to_pi(alternate_1 - original_euler));
    float distance_2 = glm::length2(wrap_to_pi(alternate_2 - original_euler));
    float distance_3 = glm::length2(wrap_to_pi(alternate_3 - original_euler));
    float distance_4 = glm::length2(wrap_to_pi(alternate_4 - original_euler));

    float best = distance_0;
    if (distance_1 < best)
    {
        best = distance_1;
        rotation_euler = alternate_1;
    }
    if (distance_2 < best)
    {
        best = distance_2;
        rotation_euler = alternate_2;
    }
    if (distance_3 < best)
    {
        best = distance_3;
        rotation_euler = alternate_3;
    }
    if (distance_4 < best)
    {
        rotation_euler = alternate_4;
    }

    rotation_euler = wrap_to_pi(rotation_euler);
}

void TransformComponent::set_rotation_euler(const glm::vec3& new_rotation_euler)
{
    rotation_euler = new_rotation_euler;
    rotation = glm::quat(rotation_euler);
}

void TransformComponent::set_scale(const glm::vec3& new_scale)
{
    scale = new_scale;
}

void TransformComponent::calculate_world_matrix(const glm::mat4& root)
{
    const auto local_matrix = glm::translate(glm::mat4(1.0f), translation)
        * glm::toMat4(rotation)
        * glm::scale(glm::mat4(1.0f), scale);

    world_matrix = root * local_matrix;
}

glm::mat4& TransformComponent::get_world_matrix()
{
    return world_matrix;
}

const glm::mat4& TransformComponent::get_world_matrix() const
{
    return world_matrix;
}

const glm::vec3& TransformComponent::get_translation() const
{
    return translation;
}


const glm::quat& TransformComponent::get_rotation() const
{
    return rotation;
}

const glm::vec3& TransformComponent::get_rotation_euler() const
{
    return rotation_euler;
}

const glm::vec3& TransformComponent::get_scale() const
{
    return scale;
}
} // portal
