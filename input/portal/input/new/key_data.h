//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/input/new/input_types.h"

namespace portal::ng
{

struct KeyData
{
    Key key;
    KeyState state = KeyState::Released;
    KeyState previous_state = KeyState::Released;
};

}
