//
// Created by thejo on 5/4/2025.
//

#include "generic_platform_input.h"
#include <GLFW/glfw3.h>

namespace portal::input
{
uint32_t GenericPlatformInput::get_keymap(std::vector<uint32_t>& key_codes, std::vector<std::string>& key_names, uint32_t max_mapping)
{
    if (key_codes.size() < max_mapping)
    {
        key_codes.resize(max_mapping);
    }
    if (key_names.size() < max_mapping)
    {
        key_names.resize(max_mapping);
    }

    uint32_t number_of_mappings = 0;

#define ADD_KEYMAP(code, name) if(number_of_mappings < max_mapping) { key_codes[number_of_mappings] = glfwGetKeyScancode(code); key_names[number_of_mappings] = name; number_of_mappings++; }

    ADD_KEYMAP(GLFW_KEY_0, "Zero");
    ADD_KEYMAP(GLFW_KEY_1, "One");
    ADD_KEYMAP(GLFW_KEY_2, "Two");
    ADD_KEYMAP(GLFW_KEY_3, "Three");
    ADD_KEYMAP(GLFW_KEY_4, "Four");
    ADD_KEYMAP(GLFW_KEY_5, "Five");
    ADD_KEYMAP(GLFW_KEY_6, "Six");
    ADD_KEYMAP(GLFW_KEY_7, "Seven");
    ADD_KEYMAP(GLFW_KEY_8, "Eight");
    ADD_KEYMAP(GLFW_KEY_9, "Nine");

    ADD_KEYMAP(GLFW_KEY_A, "A");
    ADD_KEYMAP(GLFW_KEY_B, "B");
    ADD_KEYMAP(GLFW_KEY_C, "C");
    ADD_KEYMAP(GLFW_KEY_D, "D");
    ADD_KEYMAP(GLFW_KEY_E, "E");
    ADD_KEYMAP(GLFW_KEY_F, "F");
    ADD_KEYMAP(GLFW_KEY_G, "G");
    ADD_KEYMAP(GLFW_KEY_H, "H");
    ADD_KEYMAP(GLFW_KEY_I, "I");
    ADD_KEYMAP(GLFW_KEY_J, "J");
    ADD_KEYMAP(GLFW_KEY_K, "K");
    ADD_KEYMAP(GLFW_KEY_L, "L");
    ADD_KEYMAP(GLFW_KEY_M, "M");
    ADD_KEYMAP(GLFW_KEY_N, "N");
    ADD_KEYMAP(GLFW_KEY_O, "O");
    ADD_KEYMAP(GLFW_KEY_P, "P");
    ADD_KEYMAP(GLFW_KEY_Q, "Q");
    ADD_KEYMAP(GLFW_KEY_R, "R");
    ADD_KEYMAP(GLFW_KEY_S, "S");
    ADD_KEYMAP(GLFW_KEY_T, "T");
    ADD_KEYMAP(GLFW_KEY_U, "U");
    ADD_KEYMAP(GLFW_KEY_V, "V");
    ADD_KEYMAP(GLFW_KEY_W, "W");
    ADD_KEYMAP(GLFW_KEY_X, "X");
    ADD_KEYMAP(GLFW_KEY_Y, "Y");
    ADD_KEYMAP(GLFW_KEY_Z, "Z");

    ADD_KEYMAP(GLFW_KEY_SEMICOLON, "Semicolon");
    ADD_KEYMAP(GLFW_KEY_EQUAL, "Equals");
    ADD_KEYMAP(GLFW_KEY_COMMA, "Comma");
    ADD_KEYMAP(GLFW_KEY_MINUS, "Hyphen");
    ADD_KEYMAP(GLFW_KEY_PERIOD, "Period");
    ADD_KEYMAP(GLFW_KEY_SLASH, "Slash");
    ADD_KEYMAP(GLFW_KEY_GRAVE_ACCENT, "Tilde");
    ADD_KEYMAP(GLFW_KEY_LEFT_BRACKET, "LeftBracket");
    ADD_KEYMAP(GLFW_KEY_BACKSLASH, "Backslash");
    ADD_KEYMAP(GLFW_KEY_RIGHT_BRACKET, "RightBracket");
    ADD_KEYMAP(GLFW_KEY_APOSTROPHE, "Apostrophe");
    ADD_KEYMAP(GLFW_KEY_SPACE, "SpaceBar");

    ADD_KEYMAP(GLFW_KEY_ESCAPE, "Escape");
    ADD_KEYMAP(GLFW_KEY_ENTER, "Enter");
    ADD_KEYMAP(GLFW_KEY_TAB, "Tab");
    ADD_KEYMAP(GLFW_KEY_BACKSPACE, "BackSpace");
    ADD_KEYMAP(GLFW_KEY_INSERT, "Insert");
    ADD_KEYMAP(GLFW_KEY_DELETE, "Delete");
    ADD_KEYMAP(GLFW_KEY_RIGHT, "Right");
    ADD_KEYMAP(GLFW_KEY_LEFT, "Left");
    ADD_KEYMAP(GLFW_KEY_DOWN, "Down");
    ADD_KEYMAP(GLFW_KEY_UP, "Up");
    ADD_KEYMAP(GLFW_KEY_PAGE_UP, "PageUp");
    ADD_KEYMAP(GLFW_KEY_PAGE_DOWN, "PageDown");
    ADD_KEYMAP(GLFW_KEY_HOME, "Home");
    ADD_KEYMAP(GLFW_KEY_END, "End");
    ADD_KEYMAP(GLFW_KEY_CAPS_LOCK, "CapsLock");
    ADD_KEYMAP(GLFW_KEY_SCROLL_LOCK, "ScrollLock");
    ADD_KEYMAP(GLFW_KEY_NUM_LOCK, "NumLock");
    ADD_KEYMAP(GLFW_KEY_PAUSE, "Pause");

    ADD_KEYMAP(GLFW_KEY_F1, "F1");
    ADD_KEYMAP(GLFW_KEY_F2, "F2");
    ADD_KEYMAP(GLFW_KEY_F3, "F3");
    ADD_KEYMAP(GLFW_KEY_F4, "F4");
    ADD_KEYMAP(GLFW_KEY_F5, "F5");
    ADD_KEYMAP(GLFW_KEY_F6, "F6");
    ADD_KEYMAP(GLFW_KEY_F7, "F7");
    ADD_KEYMAP(GLFW_KEY_F8, "F8");
    ADD_KEYMAP(GLFW_KEY_F9, "F9");
    ADD_KEYMAP(GLFW_KEY_F10, "F10");
    ADD_KEYMAP(GLFW_KEY_F11, "F11");
    ADD_KEYMAP(GLFW_KEY_F12, "F12");

    ADD_KEYMAP(GLFW_KEY_KP_0, "NumPadZero");
    ADD_KEYMAP(GLFW_KEY_KP_1, "NumPadOne");
    ADD_KEYMAP(GLFW_KEY_KP_2, "NumPadTwo");
    ADD_KEYMAP(GLFW_KEY_KP_3, "NumPadThree");
    ADD_KEYMAP(GLFW_KEY_KP_4, "NumPadFour");
    ADD_KEYMAP(GLFW_KEY_KP_5, "NumPadFive");
    ADD_KEYMAP(GLFW_KEY_KP_6, "NumPadSix");
    ADD_KEYMAP(GLFW_KEY_KP_7, "NumPadSeven");
    ADD_KEYMAP(GLFW_KEY_KP_8, "NumPadEight");
    ADD_KEYMAP(GLFW_KEY_KP_9, "NumPadNine");

    ADD_KEYMAP(GLFW_KEY_KP_DECIMAL, "Decimal");
    ADD_KEYMAP(GLFW_KEY_KP_DIVIDE, "Divide");
    ADD_KEYMAP(GLFW_KEY_KP_MULTIPLY, "Multiply");
    ADD_KEYMAP(GLFW_KEY_KP_SUBTRACT, "Subtract");
    ADD_KEYMAP(GLFW_KEY_KP_ADD, "Add");

    ADD_KEYMAP(GLFW_KEY_LEFT_SHIFT, "LeftShift");
    ADD_KEYMAP(GLFW_KEY_LEFT_CONTROL, "LeftControl");
    ADD_KEYMAP(GLFW_KEY_LEFT_ALT, "LeftAlt");
    ADD_KEYMAP(GLFW_KEY_LEFT_SUPER, "LeftCommand");
    ADD_KEYMAP(GLFW_KEY_RIGHT_SHIFT, "RightShift");
    ADD_KEYMAP(GLFW_KEY_RIGHT_CONTROL, "RightControl");
    ADD_KEYMAP(GLFW_KEY_RIGHT_ALT, "RightAlt");
    ADD_KEYMAP(GLFW_KEY_RIGHT_SUPER, "RightCommand");

#undef ADD_KEYMAP

    return number_of_mappings;
}


} // portal
