//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <atomic>
#include <coroutine>
#include <expected>
#include <optional>
#include <source_location>

#include "llvm/ADT/SmallVector.h"
#include "portal/core/log.h"
#include "portal/core/debug/profile.h"
#include "portal/core/jobs/job.h"
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
    Missing, // The job has not completed yet
    VoidType
};

class SuspendJob
{
public:
    constexpr bool await_ready() noexcept { return false; }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) noexcept;
    void await_resume() noexcept {};
};

class FinalizeJob
{
public:
    constexpr bool await_ready() noexcept { return false; }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) noexcept;
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
    class JobAwaiter
    {
    public:
        explicit JobAwaiter(const std::coroutine_handle<JobPromise> handle) : handle(handle) {}

        bool await_ready() noexcept;
        std::coroutine_handle<JobPromise> await_suspend(std::coroutine_handle<> caller) noexcept;

        template <typename T = void> requires (std::is_same_v<T, void>)
        void await_resume() noexcept {}

        template <typename T> requires (!std::is_same_v<T, void>)
        std::expected<T, JobResultStatus> await_resume() noexcept
        {
            PORTAL_PROF_ZONE("");
            return std::move(handle.promise().template get_result<T>());
        }

    private:
        std::coroutine_handle<JobPromise> handle;
    };

public:
    JobPromise();

    std::suspend_always initial_suspend() noexcept { return {}; }
    FinalizeJob final_suspend() noexcept { return {}; }

    void unhandled_exception() noexcept;

    template <typename Result>
    std::expected<Result, JobResultStatus> get_result()
    {
        PORTAL_PROF_ZONE();
        if (completed)
            return std::move(*static_cast<Result*>(result));
        return std::unexpected{JobResultStatus::Missing};
    }

    void add_switch_information(SwitchType type);

    template <typename Result> requires (!std::is_void_v<Result>)
    void initialize_result()
    {
        PORTAL_PROF_ZONE();
        result = allocate_result(sizeof(Result));
        new(result) Result();
    }

    template <typename Result> requires (!std::is_void_v<Result>)
    void destroy_result()
    {
        PORTAL_PROF_ZONE();
        if (result)
        {
            static_cast<Result*>(result)->~Result(); // Explicitly call destructor
            deallocate_result(result, sizeof(Result));
        }
    }

    void* operator new(size_t n) noexcept;
    void operator delete(void* ptr) noexcept;

    void set_scheduler(jobs::Scheduler* scheduler_ptr) noexcept;
    void set_counter(jobs::Counter* counter_ptr) noexcept;
    void set_continuation(std::coroutine_handle<> caller) noexcept;

    [[nodiscard]] std::coroutine_handle<> get_continuation() const noexcept { return continuation; }
    [[nodiscard]] static size_t get_allocated_size() noexcept;
    [[nodiscard]] jobs::Counter* get_counter() const noexcept { return counter; }
    [[nodiscard]] jobs::Scheduler* get_scheduler() const noexcept { return scheduler; }

    [[nodiscard]] bool is_completed() const noexcept { return completed; }


    auto operator co_await() noexcept
    {
        return JobAwaiter{std::coroutine_handle<JobPromise>::from_promise(*this)};
    }

protected:
    static void* allocate_result(size_t size) noexcept;
    static void deallocate_result(void* ptr, size_t size) noexcept;

protected:
    std::coroutine_handle<> continuation;
    void* result = nullptr;
    bool completed = false;

    jobs::Counter* counter = nullptr;
    jobs::Scheduler* scheduler = nullptr;

    llvm::SmallVector<SwitchInformation> switch_information;
};


class JobBase
{
public:
    using handle_type = std::coroutine_handle<JobPromise>;
    handle_type handle;

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
            // LOG_INFO("Destroying handle: 0x{:x}", std::bit_cast<uintptr_t>(handle.address()));
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
            // LOG_INFO("Destroying handle: 0x{:x}", std::bit_cast<uintptr_t>(handle.address()));
            handle.destroy();
        }
    }

    auto operator co_await() noexcept
    {
        return handle.promise().operator co_await();
    }

    void set_dispatched();
    void set_scheduler(jobs::Scheduler* scheduler_ptr) const noexcept;
    void set_counter(jobs::Counter* counter_ptr) const noexcept;

    [[nodiscard]] bool is_dispatched() const noexcept { return dispatched; }
    [[nodiscard]] bool is_completed() const noexcept { return handle.promise().is_completed(); }

protected:
    bool dispatched = false;
    bool owning_result = false;
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
        completed = true;
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

    void return_void() noexcept
    {
        completed = true;
    }
};

}
