//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "camera.h"

#include "transform.h"

namespace portal
{
void CameraComponent::calculate_projection()
{
    projection = glm::perspectiveFov(glm::radians(vertical_fov), static_cast<float>(width), static_cast<float>(height), near_clip, far_clip);
    inverse_projection = glm::inverse(projection);
}

void CameraComponent::calculate_view(glm::vec3 position, glm::vec3 forward_direction)
{
    view = glm::lookAt(position, position + forward_direction, glm::vec3(0, 1, 0));
    inverse_view = glm::inverse(view);
}

void CameraComponent::set_viewport_bounds(glm::uvec4 bounds)
{
    const auto new_width = static_cast<float>(bounds.z - bounds.x);
    const auto new_height = static_cast<float>(bounds.w - bounds.y);

    if (new_width != width || new_height != height)
    {
        width = static_cast<uint32_t>(new_width);
        height = static_cast<uint32_t>(new_height);

        calculate_projection();
    }
}

void CameraComponent::archive(ArchiveObject& archive) const
{
    archive.add_property("vertical_fov", vertical_fov);
    archive.add_property("near_clip", near_clip);
    archive.add_property("far_clip", far_clip);
}

CameraComponent CameraComponent::dearchive(ArchiveObject& archive)
{
    CameraComponent comp;
    archive.get_property("vertical_fov", comp.vertical_fov);
    archive.get_property("near_clip", comp.near_clip);
    archive.get_property("far_clip", comp.far_clip);
    return comp;
}

void CameraComponent::serialize(Serializer& serializer) const
{
    serializer.add_value(vertical_fov);
    serializer.add_value(near_clip);
    serializer.add_value(far_clip);
}

CameraComponent CameraComponent::deserialize(Deserializer& deserializer)
{
    CameraComponent comp;
    deserializer.get_value(comp.vertical_fov);
    deserializer.get_value(comp.near_clip);
    deserializer.get_value(comp.far_clip);
    return comp;
}
} // portal
