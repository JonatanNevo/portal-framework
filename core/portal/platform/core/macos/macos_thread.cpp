//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "macos_thread.h"

#include <pthread.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <system_error>
#include <mach/mach_error.h>

#include "portal/core/log.h"

namespace portal
{

static auto logger = Log::get_logger("Core");


void MacOSThread::set_name(const std::string& name)
{
    ThreadBase::set_name(name);
    const auto result = pthread_setname_np(name.c_str());
    if (result != 0)
        LOGGER_ERROR("Failed to set the thread name with an error: {}", std::generic_category().message(result));
}

void MacOSThread::set_affinity(const ThreadAffinity affinity, uint16_t)
{
    if (affinity == ThreadAffinity::Core || affinity == ThreadAffinity::CoreLean)
        LOGGER_DEBUG("MacOs does not support thread affinity, skipping...");
}

void MacOSThread::set_priority(const ThreadPriority priority)
{
    int policy;
    sched_param param{};


    const auto result_get = pthread_getschedparam(pthread_self(), &policy, &param);
    if (result_get != 0)
    {
        LOGGER_ERROR(
            "Failed to get thread scheduling parameters with error: {}",
            std::generic_category().message(result_get)
            );
        return;
    }

    // Get the min and max priority for the current policy
    const int min_priority = sched_get_priority_min(policy);
    const int max_priority = sched_get_priority_max(policy);
    const int mid_priority = (min_priority + max_priority) / 2;

    switch (priority)
    {
    case ThreadPriority::Low:
        param.sched_priority = min_priority;
        break;
    case ThreadPriority::Default:
        param.sched_priority = mid_priority;
        break;
    case ThreadPriority::High:
        param.sched_priority = max_priority;
        break;
    }

    const auto result_set = pthread_setschedparam(pthread_self(), policy, &param);
    if (result_set != 0)
        LOGGER_ERROR(
        "Failed to set thread priority with error: {}",
        std::generic_category().message(result_set)
        );


}

} // portal
