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

struct Counter
{
    std::atomic<size_t> count;
    std::atomic_flag blocking;
};

enum class WorkerIterationState
{
    Executed,
    FilledCache,
    EmptyQueue
};

class Scheduler
{
public:
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
    // Create a scheduler with as many hardware threads as possible
    //  0 ... No worker threads, just one main thread
    //  n ... n number of worker threads
    // -1 ... As many worker threads as cpus, -1
    explicit Scheduler(int32_t num_worker_threads, size_t job_cache_size = WorkerContext::CACHE_SIZE);
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler& operator=(Scheduler&&) = delete;

    void wait_for_counter(Counter& counter);

    // Execute all jobs in the job list, then free the job list object
    // this takes possession of the job list object, and acts as if it was
    // a blocking call.
    //
    // Once this call returns, the JobList that was given as a parameter
    // has been consumed, and you should not re-use it.
    void wait_for_jobs(std::span<JobBase> jobs, JobPriority priority = JobPriority::Normal);

    template <typename... Results>
    std::tuple<std::expected<Results, JobResultStatus>...> wait_for_jobs(
        std::tuple<Job<Results>...> jobs,
        const JobPriority priority = JobPriority::Normal
        );
    template <typename... Results>
    std::tuple<std::expected<Results, JobResultStatus>...> wait_for_jobs(Job<Results>... jobs, const JobPriority priority = JobPriority::Normal);
    template <typename Result> requires (!std::is_void_v<Result>)
    auto wait_for_jobs(const std::span<Job<Result>> jobs, const JobPriority priority = JobPriority::Normal);
    template <typename Result> requires std::is_void_v<Result>
    void wait_for_jobs(const std::span<Job<Result>> jobs, const JobPriority priority = JobPriority::Normal);
    template <typename Result> requires std::is_void_v<Result>
    void wait_for_job(Job<Result> job, const JobPriority priority = JobPriority::Normal);
    template <typename Result> requires (!std::is_void_v<Result>)
    Result wait_for_job(Job<Result> job, const JobPriority priority = JobPriority::Normal);

    void dispatch_jobs(std::span<JobBase> jobs, JobPriority priority = JobPriority::Normal, Counter* counter = nullptr);
    void dispatch_job(JobBase job, JobPriority priority = JobPriority::Normal, Counter* counter = nullptr);

    template <typename... Results>
    void dispatch_jobs(std::tuple<Job<Results...>> jobs, const JobPriority priority = JobPriority::Normal, Counter* counter = nullptr);
    template <typename Result>
    void dispatch_jobs(std::span<Job<Result>> jobs, JobPriority priority = JobPriority::Normal, Counter* counter = nullptr);
    template <typename Result>
    void dispatch_job(Job<Result> job, JobPriority priority = JobPriority::Normal, Counter* counter = nullptr);

    JobStats& get_stats() { return stats; }
    [[nodiscard]] const JobStats& get_stats() const { return stats; }
    [[nodiscard]] static size_t get_tls_worker_id() { return tls_worker_id; }

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
            (job_list.push_back(JobBase::handle_type::from_address(job.handle.address())), ...);
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
