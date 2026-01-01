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

/**
 * Status codes returned when attempting to retrieve a Job's result.
 *
 * Job results are retrieved via Job<T>::result() which returns std::expected<T, JobResultStatus>.
 * The status indicates whether the result is available or why it cannot be retrieved.
 */
enum class JobResultStatus
{
    Unknown,   ///< Unknown state (should not occur in normal operation)
    Missing,   ///< The job has not completed yet; result not available
    VoidType   ///< Attempted to retrieve result from Job<void> (which has no return value)
};

/**
 * Awaiter for suspending a Job and re-dispatching it to the scheduler.
 *
 * When a Job executes `co_await SuspendJob()`, the job is suspended and re-queued
 * in the scheduler, allowing the worker thread to process other work instead of
 * blocking. The suspended job will be resumed later when a worker picks it up.
 *
 * This is the core mechanism that enables the work-stealing scheduler's efficiency:
 * workers never idle waiting for specific jobs - they continuously process available work.
 *
 * Example:
 * @code
 * Job<void> long_operation()
 * {
 *     do_work_phase1();
 *     co_await SuspendJob();  // Yield to allow other work
 *     do_work_phase2();
 *     co_return;
 * }
 * @endcode
 */
class SuspendJob
{
public:
    constexpr bool await_ready() noexcept { return false; }

    /**
     * Suspends the job and re-queues it in the scheduler.
     * 
     * @param handle The coroutine handle to suspend
     * @return Handle to resume next (scheduler continuation)
     */
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) noexcept;
    
    void await_resume() noexcept {};
};

/**
 * Awaiter for finalizing a Job when it completes.
 *
 * Returned by JobPromise::final_suspend(), this awaiter handles cleanup when a job
 * reaches its end (via co_return). It decrements the associated Counter (if any),
 * notifies waiting threads, and returns the continuation handle to resume the caller.
 *
 * This enables automatic parent-job resumption in nested job scenarios:
 * when a child job completes, the parent that co_awaited it automatically resumes.
 */
class FinalizeJob
{
public:
    constexpr bool await_ready() noexcept { return false; }

    /**
     * Finalizes the job and returns continuation to resume.
     *
     * @param handle The completing job's coroutine handle
     * @return Continuation handle (parent job or noop_coroutine)
     */
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) noexcept;

    void await_resume() noexcept {};
};

/**
 * State transition types tracked in job execution flow for debugging and profiling.
 */
enum class SwitchType
{
    Start,
    Resume,
    Pause,
    Finish,
    Error
};

/**
 * Records a single state transition in a job's execution history.
 */
struct SwitchInformation
{
    std::thread::id thread_id;
    std::chrono::time_point<std::chrono::system_clock> time;
    SwitchType type;
};

/**
 * Promise type for the C++20 coroutine protocol used by Job<T>.
 *
 * JobPromise manages the coroutine's state, result storage, and integration with the
 * work-stealing scheduler. It implements the promise_type concept required for C++20
 * coroutines, handling suspension, resumption, result storage, and scheduler integration.
 *
 * @see ResultPromise for typed promise implementation
 * @see Job for the coroutine type using this promise
 */
class JobPromise
{
public:
    /**
     * Awaiter for suspending parent job until child job completes.
     *
     * When a Job is co_awaited (e.g., `co_await child_job()`), this awaiter:
     * 1. Checks if child is already complete (await_ready)
     * 2. Sets parent as continuation and dispatches child (await_suspend)
     * 3. Returns child's result when resumed (await_resume)
     *
     */
    class JobAwaiter
    {
    public:
        /**
         * @brief Construct awaiter for a job
         * @param handle Handle to the job being awaited
         */
        explicit JobAwaiter(const std::coroutine_handle<JobPromise> handle) : handle(handle) {}

        /**
         * Check if job is already complete (optimization to skip suspension).
         * @return true if job already finished, false if must suspend
         */
        bool await_ready() noexcept;

        /**
         * Suspend calling job and establish parent-child relationship.
         *
         * @param caller The parent job's coroutine handle
         * @return Handle to resume next (the child job to execute)
         */
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
        std::coroutine_handle<JobPromise> handle{};
    };

public:
    JobPromise();

    std::suspend_always initial_suspend() noexcept { return {}; }
    FinalizeJob final_suspend() noexcept { return {}; }

    void unhandled_exception() noexcept;

    /**
     * Retrieve the job's result value.
     *
     * @tparam Result The return type of the job
     * @return Expected containing result or JobResultStatus::Missing if incomplete
     */
    template <typename Result>
    std::expected<Result, JobResultStatus> get_result()
    {
        PORTAL_PROF_ZONE();
        if (completed)
            return std::move(*static_cast<Result*>(result));
        return std::unexpected{JobResultStatus::Missing};
    }

    /**
     * Record a state transition for profiling.
     *
     * @param type The type of state transition (Start, Resume, Pause, etc.)
     */
    void add_switch_information(SwitchType type);

    /**
     * Allocate and default-construct result storage.
     *
     * @tparam Result The return type to allocate storage for
     */
    template <typename Result> requires (!std::is_void_v<Result>)
    void initialize_result()
    {
        PORTAL_PROF_ZONE();
        result = allocate_result(sizeof(Result));
        new(result) Result();
    }

    /**
     * Destroy result storage and deallocate memory.
     *
     * @tparam Result The return type to destroy
     */
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

    /**
     * Set the scheduler for this job.
     *
     * @param scheduler_ptr Pointer to the owning scheduler
     */
    void set_scheduler(jobs::Scheduler* scheduler_ptr) noexcept;

    /**
     * Set the counter to decrement on completion.
     *
     * @param counter_ptr Pointer to the synchronization counter
     */
    void set_counter(jobs::Counter* counter_ptr) noexcept;

    /**
     * Set the coroutine to resume after this job completes.
     *
     * @param caller Handle to the continuation coroutine
     */
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

/**
 * Base class for Job<T> providing type-erased job handle.
 *
 * Move-only. Jobs must be dispatched to scheduler before destruction.
 */
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

    /** Mark job as dispatched to scheduler (prevents double-free on destruction). */
    void set_dispatched();

    /**
     * Set the scheduler for this job.
     *
     * @param scheduler_ptr Pointer to the owning scheduler
     */
    void set_scheduler(jobs::Scheduler* scheduler_ptr) const noexcept;

    /**
     * Set the counter to decrement on completion.
     *
     * @param counter_ptr Pointer to the synchronization counter
     */
    void set_counter(jobs::Counter* counter_ptr) const noexcept;

    [[nodiscard]] bool is_dispatched() const noexcept { return dispatched; }
    [[nodiscard]] bool is_completed() const noexcept { return handle.promise().is_completed(); }

protected:
    bool dispatched = false;
    bool owning_result = false;
};

template <typename Result>
class ResultPromise;

/**
 * C++20 coroutine type for asynchronous parallel work with optional return value.
 *
 * Job<T> is the primary coroutine type for the Portal Framework's work-stealing scheduler.
 * Jobs integrate with the Scheduler for parallel execution, support nested parallelism via
 * co_await, and participate in fork-join synchronization via Counter.
 *
 * Lifecycle:
 * 1. Job created suspended (via co_return from job function)
 * 2. Dispatched to Scheduler via wait_for_job() or dispatch_job()
 * 3. Executed on worker thread (may suspend/resume on different threads)
 * 4. Result retrieved via result() or automatic in co_await
 *
 * Thread Safety:
 * - Jobs can migrate between worker threads when suspended/resumed
 * - Result storage is thread-safe (accessed only after completion)
 * - Multiple threads can co_await the same job (scheduler handles synchronization)
 *
 *
 * @tparam Result Return type (use void for jobs without return values)
 *
 * @note Jobs must be dispatched to scheduler before going out of scope
 * @note Nested jobs (child co_awaited by parent) automatically chain: parent resumes when child completes
 *
 * Example (Simple Job):
 * @code
 * Job<int> compute_sum(int a, int b)
 * {
 *     co_return a + b;
 * }
 *
 * auto result = scheduler.wait_for_job(compute_sum(10, 32)); // result = 42
 * @endcode
 *
 * Example (Nested Jobs - Parent/Child):
 * @code
 * Job<int> child_work(int value)
 * {
 *     // Simulate work
 *     co_return value * 2;
 * }
 *
 * Job<int> parent_work()
 * {
 *     // Dispatch child and co_await result
 *     auto child = child_work(21);
 *     int child_result = co_await child; // Suspends parent, child executes, parent resumes with result
 *     co_return child_result; // Returns 42
 * }
 *
 * auto final = scheduler.wait_for_job(parent_work());
 * @endcode
 *
 * Example (Fork-Join Parallelism):
 * @code
 * Job<void> process_chunk(int chunk_id)
 * {
 *     // Process chunk...
 *     co_return;
 * }
 *
 * Job<void> parallel_process()
 * {
 *     Counter counter{};
 *
 *     // Dispatch 4 jobs in parallel
 *     for (int i = 0; i < 4; i++)
 *         scheduler.dispatch_job(process_chunk(i), JobPriority::Normal, &counter); // Counter is automatically incremented
 *
 *     // Wait for all chunks to complete
 *     scheduler.wait_for_counter(&counter);
 *
 *     co_return;
 * }
 * @endcode
 *
 * @see jobs::Scheduler for dispatch methods
 * @see jobs::Counter for fork-join synchronization
 * @see Task for lightweight coroutines without scheduler overhead
 */
template <typename Result = void>
class [[nodiscard]] Job final : public JobBase
{
public:
    using promise_type = ResultPromise<Result>;
    using handle_type = std::coroutine_handle<promise_type>;

    /**
     * Retrieve the job's result value.
     *
     * @return Expected containing result or JobResultStatus if job incomplete
     */
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

/**
 * Specialization of Job for void return type (no result).
 *
 * Job<void> does not allocate result storage. Calling result() always returns VoidType status.
 */
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

/**
 * Promise type for Job<Result> with non-void return value.
 *
 * Allocates result storage and stores return value via return_value().
 *
 * @tparam Result Return type
 */
template <typename Result>
class ResultPromise : public JobPromise
{
public:
    static Job<Result> get_return_object_on_allocation_failure() noexcept
    {
        return Job<Result>{nullptr};
    }

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

/**
 * Promise specialization for Job<void> with no return value.
 *
 * No result storage allocated. Completion signaled via return_void().
 */
template <>
class ResultPromise<void> : public JobPromise
{
public:
    static Job<void> get_return_object_on_allocation_failure() noexcept
    {
        return Job<void>{nullptr};
    }

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
