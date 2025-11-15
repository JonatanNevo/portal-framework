//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once


#include "llvm/ADT/SmallVector.h"
#include "portal/core/variant.h"
#include "portal/core/strings/string_id.h"
#include "portal/input/new/input_types.h"

namespace portal::ng
{

/**
 * A mapping between an action (`jump`) and a key (Key::SpaceBar)
 */
struct ActionKeyMapping
{
    StringId action;
    KeyModifierFlag modifiers;
    Key key;

    constexpr bool operator==(const ActionKeyMapping& other) const
    {
        return action == other.action && modifiers == other.modifiers && key == other.key;
    }
};


struct InputBinding
{
    // Whether the binding should consume the input or allow it to pass to other components
    bool consume_input : 1;
    // Whether the binding should execute when the game is paused
    bool execute_when_paused :1;
};

/**
 * Binding a callback to an action mapping on a given KeyState
 */
struct ActionBinding: public InputBinding
{
    StringId action;
    KeyState state;

    // TODO: use some kind of delegate system like UE
    using action_function_without_key = std::function<void()>;
    using action_function_with_key = std::function<void(Key)>;
    using action_function = std::variant<action_function_without_key, action_function_with_key>;

    action_function callback;

    void execute(const Key& key);
};

/**
 * Binding a callback to an axis mapping.
 * The callback will be called on each frame regardless of the value changing
 */
struct AxisBinding: public InputBinding
{
    StringId axis;
    float value;
    std::function<void(float)> callback;
};

/**
 * Binding a callback to an axis key.
 * The callback will be called on each frame regardless of the value changing.
 */
struct AxisKeyBinding: public InputBinding
{
    Key key;
    float value;
    std::function<void(float)> callback;
};

/**
 * Binding a callback to an axis vector.
 * The callback will be called on each frame regardless of the value changing.
 */
struct AxisVectorBinding: public InputBinding
{
    Key key;
    glm::vec3 value;
    std::function<void(glm::vec3)> callback;
};

}
