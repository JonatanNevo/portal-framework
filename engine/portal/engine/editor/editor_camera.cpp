//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "editor_camera.h"

namespace portal
{
EditorCamera::EditorCamera(const float fov, const float width, const float height, const float near_clip, const float far_clip)
    : Camera(
          glm::perspectiveFov(glm::radians(fov), width, height, near_clip, far_clip),
          glm::perspectiveFov(glm::radians(fov), width, height, far_clip, near_clip)
      ),
      focal_point(0.f),
      vertical_fov(glm::radians(fov)),
      near_clip(near_clip),
      far_clip(far_clip),
      aspect_ratio(0),
      pitch_delta(0),
      yaw_delta(0)
{
    position = {-0.51f, 0.4f, 0.74f};
    direction = {0.54, -0.42, -0.72};

    distance = glm::distance(position, focal_point);
    yaw = 3.f * glm::pi<float>() / 4.f;
    pitch = glm::pi<float>() / 4.f;

    position = calculate_position();
    const auto orientation = get_orientation();
    direction = glm::eulerAngles(orientation) * (180.0f / glm::pi<float>());
    view = glm::translate(glm::mat4(1.0f), position) * glm::toMat4(orientation);
    view = glm::inverse(view);
}

void EditorCamera::update(float dt)
{
    // TODO: get mouse position
    const glm::vec2 mouse = {};
    const auto delta = (mouse - initial_mouse_position) * 0.002f;

    if (!active)
    {
        initial_mouse_position = mouse;
        return;
    }

    if (input.is_key_pressed(Key::RightMouseButton) && !input.is_key_pressed(Key::LeftAlt))
    {
        mode = CameraMode::Flycam;
        disable_mouse();
        const float yaw_sign = get_up_direction().y < 0 ? -1.f : 1.f;

        const float speed = get_speed();

        const auto signed_up_direction = glm::vec3{0.f, yaw_sign, 0.f};

        if (input.is_key_pressed(Key::Q))
            position_delta -= dt * speed * signed_up_direction;
        if (input.is_key_pressed(Key::E))
            position_delta += dt * speed * signed_up_direction;
        if (input.is_key_pressed(Key::S))
            position_delta -= dt * speed * direction;
        if (input.is_key_pressed(Key::W))
            position_delta += dt * speed * direction;
        if (input.is_key_pressed(Key::A))
            position_delta -= dt * speed * right_direction;
        if (input.is_key_pressed(Key::D))
            position_delta += dt * speed * right_direction;

        constexpr auto rotation_speed = 0.3f;
        yaw_delta += yaw_sign * delta.x * rotation_speed;
        pitch_delta += delta.y * rotation_speed;

        right_direction = glm::cross(direction, signed_up_direction);

        direction = glm::rotate(
            glm::normalize(
                glm::cross(
                    glm::angleAxis(-pitch_delta, right_direction),
                    glm::angleAxis(-yaw_delta, signed_up_direction)
                )
            ),
            direction
        );

        const float new_distance = glm::distance(focal_point, position);
        focal_point = position + get_forward_direction() * new_distance;
        distance = new_distance;
    }
    else if (input.is_key_pressed(Key::LeftAlt))
    {
        mode = CameraMode::Arcball;

        if (input.is_key_pressed(Key::MiddleMouseButton))
        {
            disable_mouse();
            mouse_pan(delta);
        }
    }
}

glm::vec3 EditorCamera::get_up_direction() const
{
    return glm::rotate(get_orientation(), glm::vec3(0.f, 1.f, 0.f));
}

glm::vec3 EditorCamera::get_forward_direction() const
{
    return glm::rotate(get_orientation(), glm::vec3(0.f, 0.f, -1.f));
}

glm::vec3 EditorCamera::get_right_direction() const
{
    return glm::rotate(get_orientation(), glm::vec3(1.f, 0.f, 0.f));
}

glm::quat EditorCamera::get_orientation() const
{
    return glm::quat(glm::vec3(-pitch - pitch_delta, -yaw - yaw_delta, 0.f));
}

void EditorCamera::disable_mouse()
{
    throw std::runtime_error("Not implemented");
}

void EditorCamera::enable_mouse()
{
    throw std::runtime_error("Not implemented");
}

float EditorCamera::get_speed() const
{
    float speed = normal_speed;
    if (input.is_key_pressed(Key::LeftControl))
        speed /= 2 - glm::log(normal_speed);
    if (input.is_key_pressed(Key::LeftShift))
        speed *= 2 - glm::log(normal_speed);

    return glm::clamp(speed, MIN_SPEED, MAX_SPEED);
}

glm::vec2 EditorCamera::pan_speed() const
{
    const float x = glm::min(static_cast<float>(viewport_bounds.z - viewport_bounds.x) / 1000.0f, 2.4f); // max = 2.4f
    const float x_factor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

    const float y = glm::min(static_cast<float>(viewport_bounds.w - viewport_bounds.y) / 1000.0f, 2.4f); // max = 2.4f
    const float y_factor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

    return {x_factor, y_factor};
}

void EditorCamera::mouse_pan(const glm::vec2& delta)
{
    auto panning_speed = pan_speed();
    focal_point -= get_right_direction() * delta.x * panning_speed.x * distance;
    focal_point += get_up_direction() * delta.y * panning_speed.y * distance;

}

void EditorCamera::mouse_rotate(const glm::vec2& delta)
{
    constexpr auto rotation_speed = 0.3f;
    const float yaw_sign = get_up_direction().y < 0 ? -1.f : 1.f;
    yaw_delta += yaw_sign * delta.x * rotation_speed;
    pitch_delta += delta.y * rotation_speed;
}

void EditorCamera::mouse_zoom(float delta)
{
    distance -= delta * zoom_speed()
}

glm::vec3 EditorCamera::calculate_position() const
{
    return focal_point - get_forward_direction() * distance + position_delta;
}
} // portal
