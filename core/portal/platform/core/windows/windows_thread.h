//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include "portal/core/concurrency/thread_base.h"
#include "string_utils.h"

namespace portal
{
class WindowsThread final : public ThreadBase
{
public:
    template <typename F, typename... Args>
    explicit WindowsThread(const ThreadSpecification& spec, F&& f, Args&&... args)
        : ThreadBase(spec)
    {
        auto wide_name = details::to_wstring(spec.name);
        auto callable = make_callable(std::forward<F>(f), std::forward<Args>(args)...);

        thread = std::jthread(
            [
                name = spec.name,
                wide_name = wide_name,
                affinity = spec.affinity,
                priority = spec.priority,
                core = spec.core,
                callable = std::move(callable)
            ](std::stop_token st) mutable
            {
                set_name(name, wide_name);
                set_affinity(affinity, core);
                set_priority(priority);

                callable(st);
            }
        );
    }

protected:
    static void set_name(const std::string& name, const std::wstring& wide_name);
    static void set_affinity(ThreadAffinity affinity, uint16_t core);
    static void set_priority(ThreadPriority priority);
};

using Thread = WindowsThread;
} // portal
