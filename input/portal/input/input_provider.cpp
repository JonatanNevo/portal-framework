//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//
#include "input_provider.h"

namespace portal
{

void InputProvider::report_key_action(Key, KeyState, std::optional<KeyModifierFlag>) {}

void InputProvider::report_axis_change(Axis, glm::vec2) {}

}
