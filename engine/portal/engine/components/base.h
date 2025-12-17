//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/strings/string_id.h"

namespace portal
{
class InputManager;

struct NameComponent
{
    StringId name;
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
