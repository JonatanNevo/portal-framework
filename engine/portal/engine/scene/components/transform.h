//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/glm.h>

namespace portal
{
class TransformComponent
{
public:
    TransformComponent() = default;
    TransformComponent(const TransformComponent& other) = default;
    explicit TransformComponent(const glm::vec3& translation);
    TransformComponent(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale);

    void set_matrix(const glm::mat4& matrix);
    void set_translation(const glm::vec3& new_translation);
    void set_rotation(const glm::quat& new_rotation);
    void set_rotation_euler(const glm::vec3& new_rotation_euler);
    void set_scale(const glm::vec3& new_scale);

    [[nodiscard]] glm::mat4 get_matrix() const;
    [[nodiscard]] const glm::vec3& get_translation() const;
    [[nodiscard]] const glm::quat& get_rotation() const;
    [[nodiscard]] const glm::vec3& get_rotation_euler() const;
    [[nodiscard]] const glm::vec3& get_scale() const;

private:
    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale = glm::vec3(1.0f);

    // TODO: we need the euler rotation only for "human readable rotations" meaning the editor and the likes.
    // TODO: Find a way to exclude the euler rotation from runtime, maybe as a different component
    glm::vec3 rotation_euler = glm::vec3(0.0f);
};
} // portal
