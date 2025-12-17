//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once
#include "portal/application/modules/module.h"
#include "portal/core/jobs/scheduler.h"

namespace portal
{
class SchedulerModule final : public Module<>
{
public:
    explicit SchedulerModule(ModuleStack& stack, int32_t num_workers);

    [[nodiscard]] const jobs::Scheduler& get_scheduler() const { return scheduler; }
    [[nodiscard]] jobs::Scheduler& get_scheduler() { return scheduler; }

private:
    jobs::Scheduler scheduler;
};
} // portal
