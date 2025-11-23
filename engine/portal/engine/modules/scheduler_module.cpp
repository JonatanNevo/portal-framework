//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scheduler_module.h"

namespace portal
{

SchedulerModule::SchedulerModule(ModuleStack& stack, const int32_t num_workers) : Module<>(stack, STRING_ID("Scheduler")), scheduler(num_workers) {}

} // portal
