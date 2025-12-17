//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <portal/core/glm.h>

namespace portal
{
enum class ProjectionType
{
    Perspective,
    Orthographic
};

struct MainCameraTag {};

// TODO: split into structs based on projection type?
struct CameraComponent
{
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    glm::mat4 inverse_projection{1.f};
    glm::mat4 inverse_view{1.f};

    float vertical_fov = 70.f;
    float near_clip = 10000.f;
    float far_clip = 0.0175f;

    uint32_t width = 1, height = 1;

    void calculate_projection();
    void calculate_view(glm::vec3 position, glm::vec3 forward_direction);
    void set_viewport_bounds(glm::uvec4 bounds);
};
} // portal
