//
// Created by Jonatan Nevo on 21/03/2025.
//

#include "portal/core/thread.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <Windows.h>

#include "portal/core/log.h"

namespace portal
{

Thread::Thread(const std::string& name): name(name  ) {}

void Thread::set_name(const std::string& name)
{
    const HANDLE thread_handle = thread.native_handle();
    const std::wstring w_name(name.begin(), name.end());

    const auto result = SetThreadDescription(thread_handle, w_name.c_str());
    if (result == 0)
    {
        LOG_CORE_WARN("Thread", "Failed to set thread name: {}", name);
    }
    // SetThreadAffinityMask(thread_handle, 8); // TODO: Do I want this?
}

void Thread::join()
{
    if (thread.joinable())
        thread.join();
}

std::thread::id Thread::get_id() const
{
    return thread.get_id();
}

ThreadSignal::ThreadSignal(const std::string& name, const bool manual_reset)
{
    const std::wstring str(name.begin(), name.end());
    signal_handle = CreateEvent(nullptr, manual_reset, false, reinterpret_cast<LPCSTR>(str.c_str()));
}

void ThreadSignal::wait() const
{
    WaitForSingleObject(signal_handle, INFINITE);
}

void ThreadSignal::signal() const
{
    SetEvent(signal_handle);
}

void ThreadSignal::reset() const
{
    ResetEvent(signal_handle);
}

static std::thread::id s_main_thread_id;

void set_main_thread(const std::thread::id id)
{
    s_main_thread_id = id;
}

bool is_main_thread()
{
    return std::this_thread::get_id() == s_main_thread_id;
}
}
