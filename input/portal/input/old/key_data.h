//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "input_types.h"

namespace portal
{
/**
 * Per-key state storage tracking current and previous frame state.
 *
 * InputManager maintains a DenseMap<Key, KeyData> for all keys.
 */
struct KeyData
{
    Key key = Key::Invalid;
    KeyState state = KeyState::Released;
    KeyState previous_state = KeyState::Released;
};
} // namespace portal
