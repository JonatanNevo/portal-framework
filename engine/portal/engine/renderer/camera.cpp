//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "camera.h"

#include <glm/detail/type_quat.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtx/quaternion.hpp>

#include "portal/application/application.h"

namespace portal
{

bool is_key_down(const int key_code, GLFWwindow* window)
{
    const int state = glfwGetKey(window, key_code);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool is_mouse_button_down(const int button, GLFWwindow* window)
{
    const int state = glfwGetMouseButton(window, button);
    return state == GLFW_PRESS;
}

glm::vec2 get_mouse_position(GLFWwindow* window)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    return {static_cast<float>(x), static_cast<float>(y)};
}

void set_cursor_mode(const int mode, GLFWwindow* window)
{
    glfwSetInputMode(window, GLFW_CURSOR, mode);
}

Camera::Camera()
{
    forward_direction = glm::vec3{0, 0, -1};
    position = glm::vec3{0.f, 0.f, 5.f};

    recalculate_projection();
    recalculate_view();
}

void Camera::update(const float delta_time, GLFWwindow* window)
{
    auto mouse_position = get_mouse_position(window);
    auto delta = (mouse_position - last_mouse_position) * 0.002f;
    last_mouse_position = mouse_position;

    if (!is_mouse_button_down(GLFW_MOUSE_BUTTON_RIGHT, window))
    {
        set_cursor_mode(GLFW_CURSOR_NORMAL, window);
        return;
    }

    set_cursor_mode(GLFW_CURSOR_DISABLED, window);

    bool moved = false;

    constexpr glm::vec3 up_direction(0.0f, 1.0f, 0.0f);
    glm::vec3 right_direction = glm::cross(forward_direction, up_direction);

    float speed = 5.0f;

    // Movement
    if (is_key_down(GLFW_KEY_W, window))
    {
        position += forward_direction * speed * delta_time;
        moved = true;
    }
    else if (is_key_down(GLFW_KEY_S, window))
    {
        position -= forward_direction * speed * delta_time;
        moved = true;
    }
    else if (is_key_down(GLFW_KEY_A, window))
    {
        position -= right_direction * speed * delta_time;
        moved = true;
    }
    else if (is_key_down(GLFW_KEY_D, window))
    {
        position += right_direction * speed * delta_time;
        moved = true;
    }
    else if (is_key_down(GLFW_KEY_Q, window))
    {
        position -= up_direction * speed * delta_time;
        moved = true;
    }
    else if (is_key_down(GLFW_KEY_E, window))
    {
        position += up_direction * speed * delta_time;
        moved = true;
    }

    // Rotation
    if (delta.x != 0.0f || delta.y != 0.0f)
    {
        float pitch_delta = delta.y * get_rotation_speed();
        float yaw_delta = delta.x * get_rotation_speed();

        glm::quat q = glm::normalize(
            glm::cross(
                glm::angleAxis(-pitch_delta, right_direction),
                glm::angleAxis(-yaw_delta, glm::vec3(0.f, 1.0f, 0.0f))
                )
            );
        forward_direction = glm::rotate(q, forward_direction);

        moved = true;
    }

    if (moved)
    {
        recalculate_view();
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
