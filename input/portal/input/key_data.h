//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "input_types.h"

namespace portal
{

struct KeyData
{
    Key key = Key::Invalid;
    KeyState state = KeyState::Released;
    KeyState previous_state = KeyState::Released;
};

}
