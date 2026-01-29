//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "base_camera_controller.h"

#include "camera.h"
#include "transform.h"

namespace portal
{
void BaseCameraController::move_up(float scale)
{
    if (!should_move)
        return;

    constexpr glm::vec3 up_direction(0.0f, 1.0f, 0.0f);

    moved = true;
    position_delta += up_direction * scale;
}

void BaseCameraController::move_right(float scale)
{
    if (!should_move)
        return;

    constexpr glm::vec3 up_direction(0.0f, 1.0f, 0.0f);
    const glm::vec3 right_direction = glm::cross(forward_direction, up_direction);

    moved = true;
    position_delta += right_direction * scale;
}

void BaseCameraController::move_forward(float scale)
{
    if (!should_move)
        return;

    moved = true;
    position_delta += forward_direction * scale;
}

void BaseCameraController::look_to(glm::vec2 screen_space_target)
{
    if (!should_move)
        return;

    moved = true;

    if (reset_mouse_on_next_move)
    {
        // Consume the first warp after locking the cursor so we don't get a jump
        last_mouse_position = screen_space_target;
        mouse_delta = glm::vec2{0.f};
        moved = false;
        reset_mouse_on_next_move = false;
        return;
    }

    mouse_delta = (screen_space_target - last_mouse_position) * 0.002f;
    last_mouse_position = screen_space_target;
    if (mouse_delta.x != 0.0f || mouse_delta.y != 0.0f)
    {
        moved = true;
    }
}

void BaseCameraController::mark_as_moving()
{
    if (should_move == false)
        reset_mouse_on_next_move = true;

    should_move = true;
}

void BaseCameraController::mark_as_stopped_moving()
{
    should_move = false;
}

void BaseCameraController::archive(ArchiveObject& archive) const
{
    archive.add_property("forward_direction", forward_direction);
    archive.add_property("speed", speed);
    archive.add_property("rotation_speed", rotation_speed);
}


BaseCameraController BaseCameraController::dearchive(ArchiveObject& archive)
{
    BaseCameraController comp;
    archive.get_property("forward_direction", comp.forward_direction);
    archive.get_property("speed", comp.speed);
    archive.get_property("rotation_speed", comp.rotation_speed);
    return comp;
}

void BaseCameraController::serialize(Serializer& serialize) const
{
    serialize.add_value(forward_direction);
    serialize.add_value(speed);
    serialize.add_value(rotation_speed);
}

BaseCameraController BaseCameraController::deserialize(Deserializer& deserializer)
{
    BaseCameraController comp;
    deserializer.get_value(comp.forward_direction);
    deserializer.get_value(comp.speed);
    deserializer.get_value(comp.rotation_speed);
    return comp;
}

void BaseCameraController::post_serialization(Entity entity, ResourceRegistry&) const
{
    auto& transform = entity.get_component<TransformComponent>();
    auto& camera = entity.get_component<CameraComponent>();

    camera.calculate_view(transform.get_translation(), forward_direction);
}
} // portal
