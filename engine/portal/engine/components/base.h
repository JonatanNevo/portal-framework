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
    std::string_view icon = ICON_FA_CUBE;
};

struct PlayerTag
{
    int id = 0;
};

struct SceneTag {};

struct InputComponent
{
    InputManager* input_manager = nullptr;
};
}
