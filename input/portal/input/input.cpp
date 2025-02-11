//
// Created by Jonatan Nevo on 11/02/2025.
//

#include "input.h"

#include <GLFW/glfw3.h>

namespace portal
{

// TODO: Add callback support

bool Input::is_key_down(KeyCode key) const
{
    const int state = glfwGetKey(window, static_cast<int>(key));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::is_mouse_button_down(MouseButton button) const
{
    const int state = glfwGetMouseButton(window, static_cast<int>(button));
    return state == GLFW_PRESS;
}

glm::vec2 Input::get_mouse_position() const
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    return {x, y};
}

void Input::set_cursor_mode(CursorMode mode) const
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL + static_cast<int>(mode));
}

} // namespace portal
