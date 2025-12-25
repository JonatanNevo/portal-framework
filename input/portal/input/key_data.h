//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "input_types.h"

namespace portal
{
/**
 * Per-key state storage for input tracking.
 *
 * Stores current and previous frame state for a single key, enabling frame-based
 * input queries and state transition logic. The InputManager maintains a DenseMap
 * of KeyData entries (one per key from Key::Invalid+1 to Key::Max) pre-populated
 * during construction.
 *
 * The previous_state field enables detection of "just pressed" vs "held" by comparing
 * current and previous states across frames.
 *
 * @see InputManager - Maintains DenseMap<Key, KeyData> for all keys
 * @see KeyState - State enumeration values
 */
struct KeyData
{
    Key key = Key::Invalid;
    KeyState state = KeyState::Released;
    KeyState previous_state = KeyState::Released;
};
} // namespace portal
