//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/strings/string_id.h"
#include "portal/input/new/input_types.h"

namespace portal::ng
{

/**
 * A mapping between a key and a named action eg: `jump` -> Key::SpaceBar
 */
struct ActionKeyMapping
{
    /**
     * The action name, case-sensitive,  eg: 'jump'
     */
    StringId name = INVALID_STRING_ID;

    /**
     * The key to bind
     */
    Key key = Key::Invalid;

    /**
    * Either a given modifier key is needed to be pressed to activate the action
    */
    bool shift  : 1 = false;
    bool ctrl   : 1 = false;
    bool alt    : 1 = false;
    bool system : 1 = false;


    constexpr bool operator==(const ActionKeyMapping& other) const
    {
        return name == other.name && key == other.key && shift == other.shift && ctrl == other.ctrl && alt == other.alt && system == other.system;
    }
};


}
