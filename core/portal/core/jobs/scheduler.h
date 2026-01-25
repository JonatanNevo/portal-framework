//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <coroutine>
#include <cstdint>
#include <span>

#include "llvm/ADT/SmallVector.h"
#include "portal/core/debug/profile.h"
#include "portal/core/jobs/basic_coroutine.h"
#include "portal/core/jobs/job.h"
#include "portal/core/jobs/job_stats.h"
#include "portal/core/jobs/worker_queue.h"
#include "portal/platform/core/hal/thread.h"


namespace portal::jobs
{
/**
 * Synchronization primitive for fork-join parallelism.
 *
 * Counter tracks the number of dispatched jobs to enable fork-join synchronization.
 * When jobs are dispatched with a Counter, the counter is automatically incremented.
 * When each job completes, the counter is decremented. wait_for_counter() blocks until
 * the count reaches zero, indicating all jobs have completed.
 *
 *
 * Example:
 * @code
 * Counter counter{};
 *
 * // Dispatch parallel jobs
 * for (int i = 0; i < 10; i++)
 *     scheduler.dispatch_job(process_item(i), JobPriority::Normal, &counter);
 *
 * // Wait for all jobs to complete
 * scheduler.wait_for_counter(counter);
 * @endcode
 *
 * @note Counter must outlive all jobs referencing it
 * @see Scheduler::dispatch_job for job dispatch with counter
 * @see Scheduler::wait_for_counter for synchronization
 */
struct Counter
{
    std::atomic<size_t> count;
    std::atomic_flag blocking;
};

/**
 * Return status from worker_thread_iteration() / main_thread_do_work().
 */
enum class WorkerIterationState
{
    Executed,
    FilledCache,
    EmptyQueue
};

/**
 * Work-stealing scheduler for Job<T> coroutines.
 *
 * The Scheduler manages a pool of worker threads that execute Job<T> coroutines in parallel.
 * It implements a work-stealing algorithm where each worker has a local queue, and idle workers
 * can steal jobs from busy workers to achieve load balancing.
 *
 * Dispatch vs Wait:
 * - dispatch_job(s): Fire-and-forget async execution (returns immediately)
 * - wait_for_job(s): Dispatch and block until complete, returning results
 * - wait_for_counter: Block until counter reaches zero (fork-join sync)
 *
 * Job Priority:
 * - High: Processed before Normal priority jobs
 * - Normal: Standard execution priority
 * - Low: Processed after Normal priority jobs
 *
 * @note Scheduler is not copyable or movable (owns worker threads)
 * @note All jobs must complete before Scheduler destruction
 *
 * Example (Basic Usage):
 * @code
 * // Create scheduler with hardware_concurrency - 1 workers
 * Scheduler scheduler(-1);
 *
 * // Wait for single job (blocking)
 * int result = scheduler.wait_for_job(compute_value());
 *
 * // Dispatch multiple jobs async
 * Counter counter{};
 * for (int i = 0; i < 100; i++)
 *     scheduler.dispatch_job(process_item(i), JobPriority::Normal, &counter);
 * scheduler.wait_for_counter(counter);
 * @endcode
 *
 * @see Job for the coroutine type
 * @see Counter for fork-join synchronization
 */
class Scheduler
{
public:
    /**
     * Per-worker execution context with local queue and job cache.
     */
    struct WorkerContext
    {
        constexpr static size_t CACHE_SIZE = 4;
        // constexpr static size_t CACHE_SIZE = 64;
        constexpr static uint32_t STEAL_CHECK_INTERVAL = 128;
        constexpr static uint32_t SAMPLE_INTERVAL = 1000;

        WorkerQueue queue;
        std::mt19937 rng{std::random_device{}()};
        size_t worker_id;

        std::vector<JobBase::handle_type> job_cache;
        size_t cache_index = 0;

        uint32_t iterations_since_steal_check = 0;
        uint32_t iterations_since_sample = 0;
    };

public:
    /**
     * Create scheduler with specified number of worker threads.
     *
     * @param num_worker_threads Worker count:
     *   -  0: No workers, main thread only
     *   -  n: Exactly n worker threads
     *   - -1: Hardware concurrency - 1
     * @param job_cache_size Per-worker job cache size (default 4)
     */
    explicit Scheduler(int32_t num_worker_threads, size_t job_cache_size = WorkerContext::CACHE_SIZE);
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler& operator=(Scheduler&&) = delete;

    /**
     * Block until counter reaches zero (fork-join synchronization).
     *
     * @param counter Counter to wait on
     */
    void wait_for_counter(Counter& counter);

    /**
     * Dispatch jobs and block until all complete.
     *
     * @param jobs Span of type-erased jobs to execute
     * @param priority Execution priority (High, Normal, or Low)
     */
    void wait_for_jobs(std::span<JobBase> jobs, JobPriority priority = JobPriority::Normal);

    /**
     * Dispatch tuple of jobs and block until all complete, returning their results.
     *
     * @tparam Results Return types of the jobs in the tuple
     * @param jobs Tuple of Job<T> coroutines
     * @param priority Execution priority
     * @return Tuple of std::expected<T, JobResultStatus> results
     */
    template <typename... Results>
    std::tuple<std::expected<Results, JobResultStatus>...> wait_for_jobs(
        std::tuple<Job<Results>...> jobs,
        const JobPriority priority = JobPriority::Normal
    );

    /**
     * Dispatch variadic jobs and block until all complete, returning their results.
     *
     * @tparam Results Return types of the jobs
     * @param jobs Variadic pack of Job<T> coroutines
     * @param priority Execution priority
     * @return Tuple of std::expected<T, JobResultStatus> results
     */
    template <typename... Results>
    std::tuple<std::expected<Results, JobResultStatus>...> wait_for_jobs(Job<Results>... jobs, const JobPriority priority = JobPriority::Normal);

    /**
     * Dispatch span of jobs and block until all complete, collecting non-void results.
     *
     * @tparam Result Return type of the jobs
     * @param jobs Span of Job<Result> coroutines
     * @param priority Execution priority
     * @return SmallVector of results
     */
    template <typename Result> requires (!std::is_void_v<Result>)
    auto wait_for_jobs(const std::span<Job<Result>> jobs, const JobPriority priority = JobPriority::Normal);

    /**
     * Dispatch span of void jobs and block until all complete.
     *
     * @tparam Result Must be void
     * @param jobs Span of Job<void> coroutines
     * @param priority Execution priority
     */
    template <typename Result> requires std::is_void_v<Result>
    void wait_for_jobs(const std::span<Job<Result>> jobs, const JobPriority priority = JobPriority::Normal);

    /**
     * Dispatch single void job and block until complete.
     *
     * @tparam Result Must be void
     * @param job Job<void> coroutine
     * @param priority Execution priority
     */
    template <typename Result> requires std::is_void_v<Result>
    void wait_for_job(Job<Result> job, const JobPriority priority = JobPriority::Normal);

    /**
     * Dispatch single job and block until complete, returning result.
     *
     * @tparam Result Return type of the job
     * @param job Job<Result> coroutine
     * @param priority Execution priority
     * @return The job's result value
     */
    template <typename Result> requires (!std::is_void_v<Result>)
    Result wait_for_job(Job<Result> job, const JobPriority priority = JobPriority::Normal);

    /**
     * Dispatch jobs for async execution without blocking.
     *
     * @param jobs Span of type-erased jobs
     * @param priority Execution priority
     * @param counter Optional counter to track completion
     */
    void dispatch_jobs(std::span<JobBase> jobs, JobPriority priority = JobPriority::Normal, Counter* counter = nullptr);

    /**
     * Dispatch single job for async execution without blocking.
     *
     * @param job Type-erased job to execute
     * @param priority Execution priority
     * @param counter Optional counter to track completion
     */
    void dispatch_job(JobBase job, JobPriority priority = JobPriority::Normal, Counter* counter = nullptr);

    /**
     * Dispatch tuple of jobs for async execution without blocking.
     *
     * @tparam Results Return types of the jobs
     * @param jobs Tuple of Job<T> coroutines
     * @param priority Execution priority
     * @param counter Optional counter to track completion
     */
    template <typename... Results>
    void dispatch_jobs(std::tuple<Job<Results...>> jobs, const JobPriority priority = JobPriority::Normal, Counter* counter = nullptr);

    /**
     * Dispatch span of jobs for async execution without blocking.
     *
     * @tparam Result Return type of the jobs
     * @param jobs Span of Job<Result> coroutines
     * @param priority Execution priority
     * @param counter Optional counter to track completion
     */
    template <typename Result>
    void dispatch_jobs(std::span<Job<Result>> jobs, JobPriority priority = JobPriority::Normal, Counter* counter = nullptr);

    /**
     * Dispatch single typed job for async execution without blocking.
     *
     * @tparam Result Return type of the job
     * @param job Job<Result> coroutine
     * @param priority Execution priority
     * @param counter Optional counter to track completion
     */
    template <typename Result>
    void dispatch_job(Job<Result> job, JobPriority priority = JobPriority::Normal, Counter* counter = nullptr);

    JobStats& get_stats() { return stats; }
    [[nodiscard]] const JobStats& get_stats() const { return stats; }
    [[nodiscard]] static size_t get_tls_worker_id() { return tls_worker_id; }

    /**
     * Process one job from main thread
     *
     * @return State indicating whether work was executed, cache filled, or queue empty
     */
    WorkerIterationState main_thread_do_work();

private:
    void worker_thread_loop(const std::stop_token& token, size_t worker_id);
    WorkerIterationState worker_thread_iteration(WorkerContext& context);

    size_t try_steal(JobBase::handle_type* jobs, size_t max_size);
    BasicCoroutine execute_job(const JobBase::handle_type& job);

    WorkerContext& get_context();
    size_t try_dequeue_global(JobBase::handle_type* jobs, size_t max_count);

private:
    size_t num_workers;
    static thread_local size_t tls_worker_id;

    WorkerContext global_context;
    std::vector<WorkerContext> contexts;

    std::vector<Thread> threads;
    JobStats stats;
};

template <typename... Results>
std::tuple<std::expected<Results, JobResultStatus>...> Scheduler::wait_for_jobs(std::tuple<Job<Results>...> jobs, const JobPriority priority)
{
    PORTAL_PROF_ZONE();
    llvm::SmallVector<JobBase> job_list;
    job_list.reserve(std::tuple_size_v<std::tuple<Job<Results>...>>);

    std::apply(
        [&](auto&... job)
        {
            ((job_list.push_back(JobBase::handle_type::from_address(job.handle.address())), job.set_dispatched()), ...);
        },
        jobs
    );

    wait_for_jobs(job_list, priority);

    // Extract results into tuple
    return std::apply(
        [](auto&... job) -> std::tuple<std::expected<Results, JobResultStatus>...>
        {
            return std::tuple<std::expected<Results, JobResultStatus>...>{std::move(job.result())...};
        },
        jobs
    );
}


template <typename... Results>
std::tuple<std::expected<Results, JobResultStatus>...> Scheduler::wait_for_jobs(Job<Results>... jobs, const JobPriority priority)
{
    PORTAL_PROF_ZONE();
    return wait_for_jobs(std::tuple<Job<Results>...>{std::move(jobs)...}, priority);
}

template <typename Result> requires (!std::is_void_v<Result>)
auto Scheduler::wait_for_jobs(const std::span<Job<Result>> jobs, const JobPriority priority)
{
    PORTAL_PROF_ZONE();
    llvm::SmallVector<JobBase> job_list;
    job_list.reserve(jobs.size());
    for (const auto& job : jobs)
        job_list.push_back(JobBase::handle_type::from_address(job.handle.address()));

    wait_for_jobs(job_list, priority);

    llvm::SmallVector<Result> results;
    results.reserve(jobs.size());
    for (auto& job : jobs)
    {
        auto res = job.result();
        results.push_back(res.value());
    }

    return results;
}

template <typename Result> requires std::is_void_v<Result>
void Scheduler::wait_for_jobs(const std::span<Job<Result>> jobs, const JobPriority priority)
{
    PORTAL_PROF_ZONE();
    llvm::SmallVector<JobBase> job_list;
    job_list.reserve(jobs.size());
    for (const auto& job : jobs)
        job_list.push_back(JobBase::handle_type::from_address(job.handle.address()));

    wait_for_jobs(job_list, priority);
}

template <typename Result> requires std::is_void_v<Result>
void Scheduler::wait_for_job(Job<Result> job, const JobPriority priority)
{
    PORTAL_PROF_ZONE();
    wait_for_jobs(std::tuple<Job<Result>>{std::move(job)}, priority);
}

template <typename Result> requires (!std::is_void_v<Result>)
Result Scheduler::wait_for_job(Job<Result> job, const JobPriority priority)
{
    PORTAL_PROF_ZONE();
    return std::get<0>(wait_for_jobs(std::tuple<Job<Result>>{std::move(job)}, priority)).value();
}

template <typename... Results>
void Scheduler::dispatch_jobs(std::tuple<Job<Results...>> jobs, const JobPriority priority, Counter* counter)
{
    PORTAL_PROF_ZONE();
    llvm::SmallVector<JobBase> job_list;
    job_list.reserve(std::tuple_size_v<std::tuple<Job<Results...>>>);
    std::apply(
        [&](auto&... job)
        {
            (job_list.push_back(JobBase::handle_type::from_address(job.handle.address())), ...);
        },
        jobs
    );

    dispatch_jobs(job_list, priority, counter);
}

template <typename Result>
void Scheduler::dispatch_jobs(std::span<Job<Result>> jobs, JobPriority priority, Counter* counter)
{
    PORTAL_PROF_ZONE();
    llvm::SmallVector<JobBase> job_list;
    job_list.reserve(jobs.size());
    for (const auto& job : jobs)
        job_list.push_back(JobBase::handle_type::from_address(job.handle.address()));

    dispatch_jobs(job_list, priority, counter);
}

template <typename Result>
void Scheduler::dispatch_job(Job<Result> job, JobPriority priority, Counter* counter)
{
    PORTAL_PROF_ZONE();
    dispatch_job(JobBase::handle_type::from_address(job.handle.address()), priority, counter);
}
} // portal
