//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//
#include "input_event_consumer.h"

#include <optional>

namespace portal
{

void InputEventConsumer::report_key_action(Key, KeyState, std::optional<KeyModifierFlag>) {}

void InputEventConsumer::report_axis_change(Axis, glm::vec2) {}

}
