//
// Created by Jonatan Nevo on 15/03/2025.
//

#pragma once
#include <imgui.h>

#include "portal/application/input_events.h"

namespace portal::gui
{

ImGuiKey to_imgui_key(KeyCode key);
ImGuiKey to_imgui_key(TouchAction touch_action);

};