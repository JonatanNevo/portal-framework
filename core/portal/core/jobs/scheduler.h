//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <__msvc_ranges_to.hpp>
#include <coroutine>
#include <cstdint>
#include <span>
#include <concurrentqueue/concurrentqueue.h>

#include "llvm/ADT/SmallVector.h"
#include "portal/core/jobs/job.h"
#include "portal/platform/core/hal/thread.h"


namespace portal::jobs
{

struct Counter
{
    std::atomic<size_t> count;
    std::atomic_flag blocking;
};

struct WorkerQueueTraits : public moodycamel::ConcurrentQueueDefaultTraits
{
    static constexpr size_t BLOCK_SIZE = 2;
};

struct WorkerQueue
{
    moodycamel::ConcurrentQueue<JobBase::handle_type, WorkerQueueTraits> queue{1};
    std::atomic_flag has_work{};
};

class Scheduler
{
public:
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = default;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler& operator=(Scheduler&&) = delete;

    // Create a scheduler with as many hardware threads as possible
    //  0 ... No worker threads, just one main thread
    //  n ... n number of worker threads
    // -1 ... As many worker threads as cpus, -1
    static Scheduler create(int32_t num_worker_threads = 0);

    // Execute all jobs in the job list, then free the job list object
    // this takes possession of the job list object, and acts as if it was
    // a blocking call.
    //
    // Once this call returns, the JobList that was given as a parameter
    // has been consumed, and you should not re-use it.
    void wait_for_jobs(std::span<JobBase> jobs);

    template <typename... Results>
    std::tuple<std::expected<Results, JobResultStatus>...> wait_for_jobs(std::tuple<Job<Results>...> jobs)
    {
        llvm::SmallVector<JobBase> job_list;
        job_list.reserve(std::tuple_size_v<std::tuple<Job<Results>...>>);

        std::apply(
            [&](auto&... job)
            {
                (job_list.push_back(JobBase::handle_type::from_address(job.handle.address())), ...);
            },
            jobs
            );

        wait_for_jobs(job_list);

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
    std::tuple<std::expected<Results, JobResultStatus>...> wait_for_jobs(Job<Results>... jobs)
    {
        return wait_for_jobs(std::tuple<Job<Results>...>{std::move(jobs)...});
    }

    template <typename Result> requires !std::is_void_v<Result>
    auto wait_for_jobs(const std::span<Job<Result>> jobs)
    {
        llvm::SmallVector<JobBase> job_list;
        job_list.reserve(jobs.size());
        for (const auto& job : jobs)
            job_list.push_back(JobBase::handle_type::from_address(job.handle.address()));

        wait_for_jobs(job_list);

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
    void wait_for_jobs(const std::span<Job<Result>> jobs)
    {
        llvm::SmallVector<JobBase> job_list;
        job_list.reserve(jobs.size());
        for (const auto& job : jobs)
            job_list.push_back(JobBase::handle_type::from_address(job.handle.address()));

        wait_for_jobs(job_list);
    }

    template <typename Result> requires std::is_void_v<Result>
    void wait_for_job(Job<Result> job)
    {
        wait_for_jobs(std::move(job));
    }

    template<typename Result> requires !std::is_void_v<Result>
    Result wait_for_job(Job<Result> job)
    {
        return std::get<0>(wait_for_jobs(std::move(job))).value();
    }

    void dispatch_jobs(std::span<JobBase> jobs, Counter* counter = nullptr);

    void dispatch_job(JobBase job, Counter* counter = nullptr);

    template <typename... Results>
    void dispatch_jobs(std::tuple<Job<Results...>> jobs, Counter* counter = nullptr)
    {
        llvm::SmallVector<JobBase> job_list;
        job_list.reserve(std::tuple_size_v<std::tuple<Job<Results...>>>);
        std::apply(
            [&](auto&... job)
            {
                (job_list.push_back(JobBase::handle_type::from_address(job.handle.address())), ...);
            },
            jobs
            );

        dispatch_jobs(job_list, counter);
    }

    template <typename Result>
    void dispatch_jobs(std::span<Job<Result>> jobs, Counter* counter = nullptr)
    {
        llvm::SmallVector<JobBase> job_list;
        job_list.reserve(jobs.size());
        for (const auto& job : jobs)
            job_list.push_back(JobBase::handle_type::from_address(job.handle.address()));

        dispatch_jobs(job_list, counter);
    }

    template <typename Result>
    void dispatch_job(Job<Result> job, Counter* counter = nullptr)
    {
        dispatch_job(JobBase::handle_type::from_address(job.handle.address()), counter);
    }

    JobBase::handle_type pop_job();

private:
    Scheduler(size_t worker_number);

    bool try_distribute_to_worker(const JobBase::handle_type& handle);
    static void worker_thread_loop(const std::stop_token& token, WorkerQueue& worker_queue);

    // Thread-local pointer to identify if current thread is a worker and which queue it owns
    static thread_local WorkerQueue* tl_current_worker_queue;

private:
    std::vector<WorkerQueue> worker_queues;
    std::vector<Thread> threads;

    moodycamel::ConcurrentQueue<JobBase::handle_type> pending_jobs;
};


} // portal
