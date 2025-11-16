//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "camera.h"

#include <glm/detail/type_quat.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtx/quaternion.hpp>

namespace portal
{

Camera::Camera(Input& input): input(input)
{
    forward_direction = glm::vec3{0.54, -0.42, -0.72};
    position = glm::vec3{-0.51f, 0.4f, 0.74f};

    recalculate_projection();
    recalculate_view();
}

void Camera::update(const float delta_time)
{
    constexpr glm::vec3 up_direction(0.0f, 1.0f, 0.0f);
    const glm::vec3 right_direction = glm::cross(forward_direction, up_direction);

    if (should_move && (moved || std::ranges::any_of(directions, [](auto& d){ return d != 0;})))
    {
        glm::vec3 pos_delta{};
        pos_delta += directions[0] * forward_direction;
        pos_delta += directions[1] * right_direction;
        pos_delta += directions[2] * up_direction;

        position += pos_delta * speed * delta_time;

        const float pitch_delta = mouse_delta.y * get_rotation_speed();
        const float yaw_delta = mouse_delta.x * get_rotation_speed();

        const glm::quat q = glm::normalize(
            glm::cross(
                glm::angleAxis(-pitch_delta, right_direction),
                glm::angleAxis(-yaw_delta, glm::vec3(0.f, 1.0f, 0.0f))
                )
            );
        forward_direction = glm::rotate(q, forward_direction);

        recalculate_view();
        moved = false;
    }

    mouse_delta = glm::vec2{0.f};
}

void Camera::on_key_down(const Key key)
{
    if (key == Key::RightMouseButton)
    {
        input.set_cursor_mode(CursorMode::Locked);
        should_move = true;
        reset_mouse_on_next_move = true;
        return;
    }

    if (!should_move)
        return;

    if (key == Key::W)
    {
        directions[0] = 1.f;
    }
    if (key == Key::S)
    {
        directions[0] = -1.f;
    }

    if (key == Key::A)
    {
        directions[1] = -1.f;
    }
    if (key == Key::D)
    {
        directions[1] = 1.f;
    }

    if (key == Key::E || key == Key::SpaceBar)
    {
        directions[2] = 1.f;
    }
    if (key == Key::Q || key == Key::LeftShift)
    {
        directions[2] = -1.f;
    }

    moved = true;
}

void Camera::on_key_up(const Key key)
{
    if (key == Key::RightMouseButton)
    {
        input.set_cursor_mode(CursorMode::Normal);
        should_move = false;
    }

    if (key == Key::W || key == Key::S)
    {
        directions[0] = 0.f;
    }

    if (key == Key::A || key == Key::D)
    {
        directions[1] = 0.f;
    }

    if (key == Key::E || key == Key::SpaceBar || key == Key::Q || key == Key::LeftShift)
    {
        directions[2] = 0.f;
    }
}

void Camera::on_mouse_move(const glm::vec2& mouse_position)
{
    if (!should_move)
        return;

    if (reset_mouse_on_next_move)
    {
        // Consume the first warp after locking the cursor so we don't get a jump
        last_mouse_position = mouse_position;
        mouse_delta = glm::vec2{0.f};
        moved = false;
        reset_mouse_on_next_move = false;
        return;
    }

    mouse_delta = (mouse_position - last_mouse_position) * 0.002f;
    last_mouse_position = mouse_position;
    if (mouse_delta.x != 0.0f || mouse_delta.y != 0.0f)
    {
        moved = true;
    }
}

void Camera::on_resize(uint32_t new_width, uint32_t new_height)
{
    if (width == new_width && height == new_height)
        return;

    width = new_width;
    height = new_height;

    recalculate_projection();
}

const glm::mat4& Camera::get_projection() const
{
    return projection;
}

const glm::mat4& Camera::get_inverse_projection() const
{
    return inverse_projection;
}

const glm::mat4& Camera::get_view() const
{
    return view;
}

const glm::mat4& Camera::get_inverse_view() const
{
    return inverse_view;
}

const glm::vec3& Camera::get_position() const
{
    return position;
}

const glm::vec3& Camera::get_direction() const
{
    return forward_direction;
}

float Camera::get_speed() const
{
    return speed;
}

void Camera::set_speed(float new_speed)
{
    speed = new_speed;
}

float Camera::get_rotation_speed()
{
    return 0.3f;
}

void Camera::set_position(const glm::vec3& new_position) {
    position = new_position;
}

void Camera::recalculate_projection()
{
    projection = glm::perspectiveFov(glm::radians(vertical_fov), static_cast<float>(width), static_cast<float>(height), near_clip, far_clip);
    inverse_projection = glm::inverse(projection);
}

void Camera::recalculate_view()
{
    view = glm::lookAt(position, position + forward_direction, glm::vec3(0, 1, 0));
    inverse_view = glm::inverse(view);
}

} // portal
