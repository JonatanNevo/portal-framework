//
// Created by Jonatan Nevo on 11/02/2025.
//

#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "portal/input/key_codes.h"

namespace portal
{

class Input
{
public:
    // TODO: switch with some generic "Window" class to avoid coupling with GLFW
    explicit Input(GLFWwindow* window):
        window(window) {}

    bool is_key_down(KeyCode key) const;
    bool is_mouse_button_down(MouseButton button) const;

    glm::vec2 get_mouse_position() const;

    void set_cursor_mode(CursorMode mode) const;

private:
    GLFWwindow* window;
};

} // namespace portal
