//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scheduler.h"

#include "llvm/ADT/SmallVector.h"
#include "portal/core/debug/assert.h"
#include <sstream>

// Custom formatter for std::thread::id
template <>
struct fmt::formatter<std::thread::id> : fmt::formatter<std::string>
{
    auto format(std::thread::id id, format_context& ctx) const
    {
        std::ostringstream oss;
        oss << id;
        return fmt::formatter<std::string>::format(oss.str(), ctx);
    }
};

namespace portal
{

static auto logger = Log::get_logger("Core");

// Thread-local storage to identify if we're on a worker thread
thread_local jobs::WorkerQueue* jobs::Scheduler::tl_current_worker_queue = nullptr;

jobs::Scheduler jobs::Scheduler::create(int32_t num_worker_threads)
{
    if (num_worker_threads < 0)
    {
        // If negative, then this means that we must count backwards from the number of available hardware threads
        num_worker_threads = static_cast<int32_t>(std::thread::hardware_concurrency()) + num_worker_threads;
    }

    PORTAL_ASSERT(num_worker_threads >= 0, "Number of worker threads cannot be negative");
    LOGGER_INFO("Initializing scheduler with {} worker threads", num_worker_threads);

    // Create the scheduler first so worker queues are in their final location
    Scheduler scheduler(num_worker_threads);

    // Now create threads with references to worker queues in their final location
    for (int i = 0; i < num_worker_threads; ++i)
    {
        scheduler.threads.emplace_back(
            ThreadSpecification{
                .name = fmt::format("Worker Thread {}", i),
                .affinity = ThreadAffinity::CoreLean,
                .core = static_cast<uint16_t>(i)
            },
            worker_thread_loop,
            scheduler.worker_queues[i]
            );
    }

    return std::move(scheduler);
}

jobs::Scheduler::Scheduler(const size_t worker_number) : worker_queues(worker_number)
{
    threads.reserve(worker_number);
}


jobs::Scheduler::~Scheduler()
{
    // Wake up any threads that are waiting on their flag
    for (auto& [queue, has_work] : worker_queues)
    {
        has_work.test_and_set(std::memory_order_release);
        has_work.notify_one();
    }

    // We must wait until all the threads have been joined.
    // Deleting a Thread object implicitly stops (sets the stop_token) and joins.
    threads.clear();
}

void jobs::Scheduler::wait_for_jobs(const std::span<JobBase> jobs)
{
    Counter counter{};

    dispatch_jobs(jobs, &counter);

    while (counter.count.load(std::memory_order_acquire) > 0)
    {
        auto handle = pop_job();
        if (handle == nullptr)
        {
            // If we're on a worker thread, try to steal from our own queue
            if (tl_current_worker_queue != nullptr)
            {
                JobBase::handle_type worker_handle = nullptr;
                if (tl_current_worker_queue->queue.try_dequeue(worker_handle))
                {
                    if (worker_handle)
                    {
                        handle = worker_handle;
                    }
                }
            }

            // If still no job found (either not a worker or worker queue is empty)
            if (handle == nullptr)
            {
                counter.blocking.test_and_set(std::memory_order_acquire);

                // Failed to fetch a job, meaning that there are no pending jobs, only in progress, therefore, we still need to wait
                if (counter.count.load(std::memory_order_acquire) > 0)
                {
                    // Wait for the flag to be set - this is the case if any of these happen:
                    //    * the scheduler is destroyed
                    //    * the last job has completed, and all jobs are now done.
                    counter.blocking.wait(true, std::memory_order_acquire);
                }
                else
                {
                    counter.blocking.clear(std::memory_order_release);
                }

                continue;
            }
        }

        // Try to distribute to a worker thread queue
        if (try_distribute_to_worker(handle))
        {
            // Successfully offloaded to a worker thread
            continue;
        }

        // All worker threads have full queues, execute on current thread
        handle();
    }
}

void jobs::Scheduler::dispatch_jobs(const std::span<JobBase> jobs, Counter* counter)
{
    llvm::SmallVector<JobBase::handle_type> job_pointers;
    job_pointers.reserve(jobs.size());

    for (auto& job : jobs)
    {
        job.dispatched = true;
        auto& [scheduler, promise_counter, _] = job.handle.promise();
        scheduler = this;
        if (counter)
            promise_counter = counter;
        job_pointers.push_back(job.handle);
    }

    pending_jobs.enqueue_bulk(job_pointers.begin(), job_pointers.size());
    if (counter)
        counter->count.fetch_add(jobs.size(), std::memory_order_release);
}

void jobs::Scheduler::dispatch_job(JobBase job, Counter* counter)
{
    dispatch_jobs(std::span{&job, 1}, counter);
}

JobBase::handle_type jobs::Scheduler::pop_job()
{
    JobBase::handle_type handle = nullptr;
    pending_jobs.try_dequeue(handle);

    return handle;
}

bool jobs::Scheduler::try_distribute_to_worker(const JobBase::handle_type& handle)
{
    if (worker_queues.empty())
        return false;

    int worker_idx = 0;
    for (auto& worker_queue : worker_queues)
    {
        auto& [queue, has_work] = worker_queue;
        if (&worker_queue != tl_current_worker_queue && queue.try_enqueue(handle))
        {
            has_work.test_and_set(std::memory_order_release);
            has_work.notify_one();
            return true;
        }
        worker_idx++;
    }

    // No free worker thead was found, defaulting to scheduler's thread
    return false;
}

void jobs::Scheduler::worker_thread_loop(const std::stop_token& token, WorkerQueue& worker_queue)
{
    // Set thread-local to identify this as a worker thread and which queue it owns
    tl_current_worker_queue = &worker_queue;

    while (!token.stop_requested())
    {
        JobBase::handle_type handle = nullptr;

        // Try to dequeue from our own queue
        if (worker_queue.queue.try_dequeue(handle))
        {
            if (handle)
            {
                handle.resume();
            }
            continue;
        }

        // No work available in our queue, wait for notification from try_distribute_to_worker
        worker_queue.has_work.wait(false, std::memory_order_acquire);

        // After wake up, clear the flag for next wait cycle
        worker_queue.has_work.clear(std::memory_order_release);
    }

    tl_current_worker_queue = nullptr;
    LOGGER_TRACE("[worker_thread] Thread {} stopped", std::this_thread::get_id());
}
} // portal
