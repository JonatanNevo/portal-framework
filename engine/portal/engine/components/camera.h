//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <glm/vec4.hpp>

#include "portal/engine/renderer/camera.h"

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
    ProjectionType projection_type = ProjectionType::Perspective;

    float perspective_vertical_fov = 70.f;
    float perspective_near_clip = 10000.f;
    float perspective_far_clip = 0.1f;

    float orthographic_size = 10.f;
    float orthographic_near_clip = 1.f;
    float orthographic_far_clip = -1.f;
    glm::uvec4 viewport_bounds = {0, 0, 0, 0};

    ng::Camera camera{};

    void set_perspective(float fov, float near_clip, float far_clip);
    void set_orthographic(float size, float near_clip, float far_clip);
    void set_viewport_bounds(glm::uvec4 bounds);
};

} // portal