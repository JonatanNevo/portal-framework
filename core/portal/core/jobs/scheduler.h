//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <coroutine>
#include <cstdint>
#include <span>
#include <concurrentqueue/concurrentqueue.h>

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
    moodycamel::ConcurrentQueue<void*, WorkerQueueTraits> queue{1};
    std::atomic_flag has_work{};
};

class Scheduler
{
    using HandleType = std::coroutine_handle<JobPromise>;

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
    void wait_for_jobs(std::span<const Job> jobs);
    void wait_for_jobs(std::vector<Job>&& jobs);
    void wait_for_jobs(std::initializer_list<Job> jobs);
    void wait_for_job(Job job);

    void dispatch_jobs(std::span<const Job> jobs, Counter* counter = nullptr);
    void dispatch_jobs(std::initializer_list<Job> jobs, Counter* counter = nullptr);
    void dispatch_job(Job job, Counter* counter = nullptr);

    HandleType pop_job();

private:
    Scheduler(size_t worker_number);

    bool try_distribute_to_worker(const HandleType& handle);
    static void worker_thread_loop(const std::stop_token& token, WorkerQueue& worker_queue);

    // Thread-local pointer to identify if current thread is a worker and which queue it owns
    static thread_local WorkerQueue* tl_current_worker_queue;

private:
    std::vector<WorkerQueue> worker_queues;
    std::vector<Thread> threads;

    moodycamel::ConcurrentQueue<void*> pending_jobs;
};

} // portal
