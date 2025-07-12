//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "bindings.h"

namespace portal
{

void input::ActionBinding::generate_new_handle()
{
    static int32_t global_handle = 1;
    handle = global_handle++;
}

}