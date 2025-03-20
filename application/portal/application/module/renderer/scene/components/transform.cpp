//
// Created by Jonatan Nevo on 11/03/2025.
//

#include "transform.h"

#include <glm/gtx/matrix_decompose.hpp>

#include "portal/application/module/renderer/scene/node.h"

namespace portal::scene_graph
{
Transform::Transform(Node& node): node(node)
{}

std::type_index Transform::get_type()
{
    return typeid(Transform);
}

Node& Transform::get_node() const
{
    return node;
}

void Transform::set_translation(const glm::vec3& translation)
{
    this->translation = translation;
    invalidate_world_matrix();
}

void Transform::set_rotation(const glm::quat& rotation)
{
    this->rotation = rotation;
    invalidate_world_matrix();
}

void Transform::set_scale(const glm::vec3& scale)
{
    this->scale = scale;
    invalidate_world_matrix();
}

void Transform::set_matrix(const glm::mat4& matrix)
{
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(matrix, scale, rotation, translation, skew, perspective);
    invalidate_world_matrix();
}

const glm::vec3& Transform::get_translation() const
{
    return translation;
}

const glm::quat& Transform::get_rotation() const
{
    return rotation;
}

const glm::vec3& Transform::get_scale() const
{
    return scale;
}

glm::mat4 Transform::get_matrix() const
{
    return glm::translate(glm::mat4(1.0), translation) *
        glm::mat4_cast(rotation) *
        glm::scale(glm::mat4(1.0), scale);
}

glm::mat4 Transform::get_world_matrix()
{
    update_world_transform();
    return world_matrix;
}

void Transform::invalidate_world_matrix()
{
    update_world_matrix = true;
}

void Transform::update_world_transform()
{
    if (!update_world_matrix)
        return;

    world_matrix = get_matrix();

    auto parent = node.get_parent();
    if (parent)
    {
        auto& transform = parent->get_component<Transform>();
        world_matrix = transform.get_world_matrix() * world_matrix;
    }

    update_world_matrix = false;
}
} // portal
