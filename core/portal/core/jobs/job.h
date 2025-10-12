//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <atomic>
#include <coroutine>
#include <optional>

#include "portal/core/log.h"

namespace portal
{
class JobList;

namespace jobs {
    class Scheduler;
    struct Counter;
}

struct JobPromise;

struct [[nodiscard]] Job : std::coroutine_handle<JobPromise>
{
    using promise_type = JobPromise;
};

struct SuspendJob
{
    constexpr bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<JobPromise> handle) noexcept;
    void await_resume() noexcept {};
};

struct FinalizeJob
{
    constexpr bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<JobPromise> handle) noexcept;
    void await_resume() noexcept {};
};

struct JobPromise
{
    Job get_return_object()
    {
        return { Job::from_promise(*this) };
    }

    std::suspend_always initial_suspend() noexcept { return {}; }
    FinalizeJob final_suspend() noexcept { return {}; }

    void return_void() noexcept {}

    void unhandled_exception() noexcept
    {
        LOG_ERROR_TAG("Task", "Unhandled exception in task");
    }

    jobs::Scheduler* scheduler = nullptr;
    jobs::Counter* counter;
};

}

/**
namespace portal
{
class JobList;

namespace jobs {
    class Scheduler;
    struct Counter;
}


template <typename Result>
struct ResultPromise;

template <typename Result = void>
struct [[nodiscard]] Job : std::coroutine_handle<ResultPromise<Result>>
{
    using promise_type = ResultPromise<Result>;
};

struct SuspendJob
{
    constexpr bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<JobPromise> handle) noexcept;
    void await_resume() noexcept {};
};

struct FinalizeJob
{
    constexpr bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<JobPromise> handle) noexcept;
    void await_resume() noexcept {};
};

struct JobPromise
{
    std::suspend_always initial_suspend() noexcept { return {}; }
    FinalizeJob final_suspend() noexcept { return {}; }

    void unhandled_exception() noexcept
    {
        LOG_ERROR_TAG("Task", "Unhandled exception in task");
    }

    jobs::Scheduler* scheduler = nullptr;
    jobs::Counter* counter;
};

template <typename Result>
struct ResultPromise: JobPromise
{
    Result result;

    Job<Result> get_return_object()
    {
        return { Job<Result>::from_promise(*this) };
    }

    void return_value(Result value)
    {
        result = std::move(value);
    }
};

template <>
struct ResultPromise<void>: JobPromise
{
    Job<void> get_return_object()
    {
        return { Job<void>::from_promise(*this) };
    }

    void return_void() noexcept {}
};


}
*/
