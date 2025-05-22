//
// Created by thejo on 5/19/2025.
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