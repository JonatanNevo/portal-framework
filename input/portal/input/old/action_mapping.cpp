//
// Copyright © 2026 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "action_mapping.h"
#include "portal/core/strings/string_utils.h"

namespace portal
{
auto logger = Log::get_logger("portal::input");

void ActionMap::bind(StringId name, InputBinding binding) {
    if (action_to_binding.contains(name))
    {
        LOGGER_ERROR("Failed to bind '{}' to '{}', action name already registered", name, binding.key);
        return;
    }

    if (key_to_action.contains(binding.key))
    {
        LOGGER_ERROR("Failed to bind '{}' to '{}', key already registered", name, binding.key);
        return;
    }
    action_to_binding[name] = binding;
    key_to_action[binding.key] = name;
}
}
