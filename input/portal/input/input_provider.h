//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include <portal/core/glm.h>
#include "portal/input/input_types.h"

namespace portal
{

/**
 * An internal class for type-erased communication between the platform message loop and the input system
 */
class InputProvider
{
public:
    virtual ~InputProvider() = default;

    virtual void report_key_action(Key key, KeyState state,std::optional<KeyModifierFlag> modifiers);
    virtual void report_axis_change(Axis axis, glm::vec2 value);
};
}
