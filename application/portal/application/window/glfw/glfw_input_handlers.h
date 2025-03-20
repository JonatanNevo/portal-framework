//
// Created by Jonatan Nevo on 01/03/2025.
//

#pragma once


#include "portal/application/input_events.h"

namespace portal
{

KeyCode translate_key_code(const int key);
KeyAction translate_key_action(const int action);
MouseButton translate_mouse_button(int button);
MouseAction translate_mouse_action(const int action);

}
