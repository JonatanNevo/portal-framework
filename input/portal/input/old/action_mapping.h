//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>

#include "../keys.h"
#include "portal/core/strings/string_id.h"

namespace portal
{
enum class ActionType
{
    Button,
    Axis,
    Axis2D
};

struct InputBinding
{
    Key key{};
    float scale = 1.f;
};

struct InputAction
{
    StringId name;
    ActionType type;
    llvm::SmallVector<InputBinding, 4> bindings;
};

class ActionMap
{
public:
    void bind(StringId name, InputBinding binding);
    [[nodiscard]] bool is_pressed(StringId action) const;
    [[nodiscard]] bool just_pressed(StringId action) const;
    [[nodiscard]] float axis(StringId action) const;
    [[nodiscard]] glm::vec2 axis_2d(StringId action) const;

    void update(InputAction);

private:
    llvm::DenseMap<StringId, InputBinding> action_to_binding;
    llvm::DenseMap<Key, StringId> key_to_action;
};
}
