//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "windows_thread.h"

#include <windows.h>
#include <processthreadsapi.h>
#include <system_error>

#include "portal/core/log.h"

namespace portal
{
static auto logger = Log::get_logger("Core");

void WindowsThread::set_name(const std::string& name, const std::wstring& wide_name)
{
    ThreadBase::set_name(name);
    const auto result = SetThreadDescription(GetCurrentThread(), wide_name.c_str());
    if (FAILED(result))
        LOGGER_ERROR("Failed to set thread name with result: {}", std::system_category().message(result));
}

void WindowsThread::set_affinity(const ThreadAffinity affinity, const uint16_t core)
{
    if (affinity == ThreadAffinity::Core)
    {
        const DWORD_PTR affinity_mask = DWORD_PTR{1} << core;
        const auto result = SetThreadAffinityMask(GetCurrentThread(), affinity_mask);
        if (result == 0)
        {
            const auto error = GetLastError();
            LOGGER_ERROR("Failed to set thread affinity with result: {}", std::system_category().message(error));
        }
    }
    if (affinity == ThreadAffinity::CoreLean)
    {
        const auto result = SetThreadIdealProcessor(GetCurrentThread(), core);
        if (result == static_cast<DWORD>(-1))
        {
            const auto error = GetLastError();
            LOGGER_ERROR("Failed to set thread affinity with result: {}", std::system_category().message(error));
        }
    }
}


void WindowsThread::set_priority(const ThreadPriority priority)
{
    int priority_num = THREAD_PRIORITY_NORMAL;
    switch (priority)
    {
    case ThreadPriority::Low:
        priority_num = THREAD_PRIORITY_BELOW_NORMAL;
        break;
    case ThreadPriority::Default:
        priority_num = THREAD_PRIORITY_NORMAL;
        break;
    case ThreadPriority::High:
        priority_num = THREAD_PRIORITY_HIGHEST;
        break;
    }

    const auto result = SetThreadPriority(GetCurrentThread(), priority_num);
    if (!result)
    {
        const auto error = GetLastError();
        LOGGER_ERROR("Failed to set thread priority with result: {}", std::system_category().message(error));
    }
}
} // portal
