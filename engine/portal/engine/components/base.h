//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/strings/string_id.h"
#include "portal/third_party/font_awsome/IconsFontAwesome6.h"

namespace portal
{
class InputManager;

struct NameComponent
{
    StringId name;
    const char* icon = ICON_FA_CUBE;
};

struct PlayerTag
{
    const int id = 0;
};

struct SceneTag {};

struct InputComponent
{
    InputManager* input_manager = nullptr;
};
}
