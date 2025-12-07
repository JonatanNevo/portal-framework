//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "camera.h"

namespace portal
{
void CameraComponent::set_perspective(const float fov, const float near_clip, const float far_clip)
{
    projection_type = ProjectionType::Perspective;
    perspective_vertical_fov = fov;
    perspective_near_clip = near_clip;
    perspective_far_clip = far_clip;
}

void CameraComponent::set_orthographic(const float size, const float near_clip, const float far_clip)
{
    projection_type = ProjectionType::Orthographic;
    orthographic_size = size;
    orthographic_near_clip = near_clip;
    orthographic_far_clip = far_clip;
}

void CameraComponent::set_viewport_bounds(glm::uvec4 bounds)
{
    viewport_bounds = {bounds.x, bounds.y, bounds.z, bounds.w};
    const float width = static_cast<float>(bounds.z - bounds.x);
    const float height = static_cast<float>(bounds.w - bounds.y);

    switch (projection_type)
    {
    case ProjectionType::Perspective:
        camera.set_perspective_projection(glm::radians(perspective_vertical_fov), width, height, perspective_near_clip, perspective_far_clip);
        break;
    case ProjectionType::Orthographic:
        const float aspect = width / height;
        const float orto_width = aspect * orthographic_size;
        const float orto_height = orthographic_size;
        camera.set_orthographic_projection(orto_width, orto_height, orthographic_near_clip, orthographic_far_clip);
        break;
    }
}
} // portal
