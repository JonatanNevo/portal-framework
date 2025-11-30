//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "linux_thread.h"
#include <pthread.h>

#include "portal/core/log.h"

namespace portal
{
static auto logger = Log::get_logger("Core");

void LinuxThread::set_name(const std::string& name)
{
    ThreadBase::set_name(name);
    const auto result = pthread_setname_np(pthread_self(), name.c_str());
    if (result != 0)
        LOGGER_ERROR("Failed to set the thread name with an error: {}", std::generic_category().message(result));
}

void LinuxThread::set_affinity(const ThreadAffinity affinity, uint16_t core)
{
    if (affinity == ThreadAffinity::Core || affinity == ThreadAffinity::CoreLean)
    {
        if (affinity == ThreadAffinity::CoreLean)
            LOGGER_WARN("Linux does not support lean affinity, using hard affinity instead");

        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core, &cpuset);
        const auto result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        if (result != 0)
            LOGGER_ERROR("Failed to set thread affinity with result: {}", std::generic_category().message(result));
    }
}

void LinuxThread::set_priority(ThreadPriority priority)
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

    const int min_priority = sched_get_priority_min(policy);
    const int max_priority = sched_get_priority_max(policy);
    const int mid_priority = param.sched_priority;

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
    {
        LOGGER_ERROR(
            "Failed to set thread priority with error: {}",
            std::generic_category().message(result_set)
        );
    }
}
} // portal
