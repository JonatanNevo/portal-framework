//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "bindings.h"

namespace portal::ng
{

void ActionBinding::execute(const Key& key)
{
    match(
        callback,
        [](const action_function_without_key& func)
        {
            func();
        },
        [&key](const action_function_with_key& func)
        {
            func(key);
        }
        );
}

}
