//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <atomic>
#include <coroutine>
#include <expected>
#include <optional>
#include <memory_resource>

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Allocator.h"
#include "portal/core/log.h"
#include "portal/core/jobs/job.h"
#include "portal/core/memory/pool_allocator.h"
#include "portal/core/memory/stack_allocator.h"

namespace portal
{
class JobList;

namespace jobs
{
    class Scheduler;
    struct Counter;
}

enum class JobResultStatus
{
    Unknown,
    Missing,
    VoidType
};


class SuspendJob
{
public:
    constexpr bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) noexcept;
    void await_resume() noexcept {};
};

class FinalizeJob
{
public:
    constexpr bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) noexcept;
    void await_resume() noexcept {};
};

enum class SwitchType
{
    Start,
    Resume,
    Pause,
    Finish,
    Error
};

struct SwitchInformation
{
    std::thread::id thread_id;
    std::chrono::time_point<std::chrono::system_clock> time;
    SwitchType type;
};

class JobPromise
{
public:
    jobs::Scheduler* scheduler = nullptr;
    jobs::Counter* counter = nullptr;
    void* result = nullptr;
    std::vector<SwitchInformation> switch_information;

    JobPromise();

    std::suspend_always initial_suspend() noexcept { return {}; }
    FinalizeJob final_suspend() noexcept { return {}; }

    void unhandled_exception() noexcept;

    template <typename Result>
    std::expected<Result, JobResultStatus> get_result()
    {
        return std::move(*static_cast<Result*>(result));
    }

    void add_switch_information(SwitchType type);

    template <typename Result> requires (!std::is_void_v<Result>)
    void initialize_result()
    {
        result = allocate_result(sizeof(Result));
        new(result) Result();
    }

    template <typename Result> requires (!std::is_void_v<Result>)
    void destroy_result()
    {
        if (result)
        {
            static_cast<Result*>(result)->~Result(); // Explicitly call destructor
            deallocate_result(result, sizeof(Result));
        }
    }

    void* operator new(size_t n) noexcept;
    void operator delete(void* ptr) noexcept;

    [[nodiscard]] static size_t get_allocated_size() noexcept;

protected:
    static void* allocate_result(size_t size) noexcept;
    static void deallocate_result(void* ptr, size_t size) noexcept;
};


class JobBase
{
public:
    using handle_type = std::coroutine_handle<JobPromise>;
    handle_type handle;

    bool dispatched = false;
    bool owning_result = false;

    JobBase(const handle_type handle) : handle(handle) {}

    JobBase(const JobBase&) = delete;
    JobBase(JobBase&& other) noexcept : handle(std::exchange(other.handle, nullptr)), owning_result(std::exchange(other.owning_result, false)) {}

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
        owning_result = std::exchange(other.owning_result, false);

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

template <typename Result>
class ResultPromise;

template <typename Result = void>
class [[nodiscard]] Job final : public JobBase
{
public:
    using promise_type = ResultPromise<Result>;
    using handle_type = std::coroutine_handle<promise_type>;

    std::expected<Result, JobResultStatus> result()
    {
        auto result_handle = handle_type::from_address(handle.address());
        return result_handle.promise().template get_result<Result>();
    }

    Job(handle_type result_handle) : JobBase(JobBase::handle_type::from_address(result_handle.address()))
    {
        handle.promise().template initialize_result<Result>();
        owning_result = true;
    };

    Job(Job&& other) noexcept : JobBase(std::move(other)) {}

    Job& operator=(Job&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (owning_result)
        {
            handle.promise().template destroy_result<Result>();
        }

        JobBase::operator=(std::move(other));

        return *this;
    };

    ~Job() override
    {
        if (owning_result)
        {
            handle.promise().template destroy_result<Result>();
        }
    }
};

template <>
class [[nodiscard]] Job<void> final : public JobBase
{
public:
    using promise_type = ResultPromise<void>;
    using handle_type = std::coroutine_handle<promise_type>;

    std::expected<void, JobResultStatus> result()
    {
        return std::unexpected{JobResultStatus::VoidType};
    }

    Job(const handle_type result_handle) : JobBase(JobBase::handle_type::from_address(result_handle.address())) {};

    Job(Job&& other) noexcept = default;
    Job& operator=(Job&& other) noexcept = default;
};

template <typename Result>
class ResultPromise : public JobPromise
{
public:
    Job<Result> get_return_object()
    {
        const auto handle = std::coroutine_handle<ResultPromise>::from_promise(*this);
        return {handle};
    }

    void return_value(Result value)
    {
        *static_cast<Result*>(result) = std::move(value);
    }
};

template <>
class ResultPromise<void> : public JobPromise
{
public:
    Job<void> get_return_object()
    {
        const auto handle = std::coroutine_handle<ResultPromise>::from_promise(*this);
        return Job{handle};
    }

    void return_void() noexcept {}
};

}
