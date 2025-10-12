//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <atomic>
#include <coroutine>
#include <expected>
#include <optional>

#include "portal/core/log.h"

namespace portal
{
class JobList;

namespace jobs {
    class Scheduler;
    struct Counter;
}

enum class JobResultStatus
{
    Unknown,
    Missing,
    VoidType
};


struct SuspendJob
{
    constexpr bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) noexcept;
    void await_resume() noexcept {};
};

struct FinalizeJob
{
    constexpr bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) noexcept;
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
struct ResultPromise;

struct JobBase
{
    using handle_type = std::coroutine_handle<JobPromise>;
    handle_type handle;

    bool dispatched = false;

    JobBase(const handle_type handle): handle(handle) {}
    JobBase(const JobBase&) = delete;
    JobBase(JobBase&& other) noexcept: handle(std::exchange(other.handle, nullptr)) {}

    JobBase& operator=(const JobBase&) = delete;
    JobBase& operator=(JobBase&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (handle && !dispatched)
        {
            handle.destroy();
        }

        dispatched = other.dispatched;
        handle = std::exchange(other.handle, nullptr);
        return *this;
    }

    virtual ~JobBase()
    {
        if (handle && !dispatched)
        {
            handle.destroy();
        }
    }
};

template <typename Result = void>
struct [[nodiscard]] Job final : JobBase
{
    using promise_type = ResultPromise<Result>;
    using handle_type = std::coroutine_handle<promise_type>;

    std::expected<Result, JobResultStatus> result() const
    {
        auto result_handle = handle_type::from_address(handle.address());
        return result_handle.promise().get_result();
    }

    Job(handle_type result_handle): JobBase(JobBase::handle_type::from_address(result_handle.address())) {};
};


template <typename Result>
struct ResultPromise: JobPromise
{
    Result result;

    Job<Result> get_return_object()
    {
        return { Job<Result>::handle_type::from_promise(*this) };
    }

    void return_value(Result value)
    {
        result = std::move(value);
    }

    std::expected<Result, JobResultStatus> get_result() const
    {
        if constexpr (std::is_move_assignable_v<Result> || std::is_move_constructible_v<Result>)
            return std::move(result);

        return result;
    }
};

template <>
struct ResultPromise<void>: JobPromise
{
    Job<void> get_return_object()
    {
        return { Job<void>::handle_type::from_promise(*this) };
    }

    std::expected<void, JobResultStatus> get_result() const
    {
        return std::unexpected { JobResultStatus::VoidType };
    }

    void return_void() noexcept {}
};


}
