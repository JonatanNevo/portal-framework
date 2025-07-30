//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/glm.h>

#include "portal/input/input.h"

namespace portal
{

class Camera
{
public:
    Camera();

    // Workaround until I fix the input system
    void update(float delta_time, GLFWwindow* window);
    void on_resize(uint32_t new_width, uint32_t new_height);

    const glm::mat4& get_projection() const;
    const glm::mat4& get_inverse_projection() const;
    const glm::mat4& get_view() const;
    const glm::mat4& get_inverse_view() const;

    const glm::vec3& get_position() const;
    const glm::vec3& get_direction() const;

    float get_rotation_speed();

    void set_position(const glm::vec3& new_position);

private:
    void recalculate_projection();
    void recalculate_view();

private:
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    glm::mat4 inverse_projection{1.f};
    glm::mat4 inverse_view{1.f};

    float vertical_fov = 70.f;
    float near_clip = 10000.f;
    float far_clip = 0.1f;

    glm::vec3 position{0.f, 0.f, 0.f};
    glm::vec3 forward_direction{0.f, 0.f, 0.f};

    glm::vec2 last_mouse_position{0.f, 0.f};
    uint32_t width = 1, height = 1;
};
} // portal
