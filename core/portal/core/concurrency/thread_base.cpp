//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "thread_base.h"

#include "portal/core/debug/profile.h"

namespace portal
{
ThreadBase::~ThreadBase()
{
    try_cancel_and_join();
}

ThreadBase& ThreadBase::operator=(ThreadBase&& other) noexcept
{
    if (this == &other)
        return *this;

    try_cancel_and_join();
    thread = std::move(other.thread);
    spec = std::move(other.spec);
    return *this;
}

bool ThreadBase::joinable() const noexcept
{
    return thread.joinable();
}

void ThreadBase::join()
{
    thread.join();
}

void ThreadBase::detach()
{
    thread.detach();
}

std::thread::id ThreadBase::get_id() const noexcept
{
    return thread.get_id();
}

std::string ThreadBase::get_name() const noexcept
{
    return spec.name;
}

bool ThreadBase::request_stop() noexcept
{
    return thread.request_stop();
}


void ThreadBase::try_cancel_and_join()
{
    if (thread.joinable())
    {
        thread.request_stop();
        thread.join();
    }
}

void ThreadBase::set_name([[maybe_unused]] const std::string& name)
{
    PORTAL_NAME_THREAD(name.c_str());
}
}
