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
    return Scheduler{static_cast<size_t>(num_worker_threads)};
}

JobBase::handle_type jobs::Scheduler::try_dequeue_job() const
{
    JobBase::handle_type handle = nullptr;
    pending_jobs->try_dequeue(handle);
    return handle;
}

jobs::Scheduler::Scheduler(const size_t worker_number): pending_jobs(std::make_shared<moodycamel::BlockingConcurrentQueue<JobBase::handle_type>>())
{
    threads.reserve(worker_number);
    for (int i = 0; i < worker_number; ++i)
    {
        threads.emplace_back(
            ThreadSpecification{
                .name = fmt::format("Worker Thread {}", i),
                .affinity = ThreadAffinity::CoreLean,
                .core = static_cast<uint16_t>(i)
            },
            [&](const std::stop_token& stop_token)
            {
                worker_thread_loop(stop_token, pending_jobs);
            }
            );
    }
}

Job<> empty_job()
{
    co_return;
}


jobs::Scheduler::~Scheduler()
{
    if (!threads.empty())
    {
        llvm::SmallVector<Job<>> empty_jobs;
        for (auto& thread : threads)
        {
            thread.request_stop();
            empty_jobs.push_back(empty_job());
        }

        // Dispatch an empty job to awaken worker threads
        dispatch_jobs(std::span{empty_jobs.data(), empty_jobs.size()});

        for (auto& thread : threads)
        {
            thread.join();
        }
    }
}


void jobs::Scheduler::wait_for_jobs(const std::span<JobBase> jobs)
{
    Counter counter{};

    dispatch_jobs(jobs, &counter);

    while (counter.count.load(std::memory_order_acquire) > 0)
    {
        auto handle = try_dequeue_job();
        if (handle == nullptr)
        {
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

        handle.promise().add_switch_information(SwitchType::Resume);
        handle.resume();
    }
}

void jobs::Scheduler::dispatch_jobs(const std::span<JobBase> jobs, Counter* counter)
{
    llvm::SmallVector<JobBase::handle_type> job_pointers;
    job_pointers.reserve(jobs.size());

    for (auto& job : jobs)
    {
        job.dispatched = true;
        auto& [scheduler, promise_counter, _, __] = job.handle.promise();
        scheduler = this;
        if (counter)
            promise_counter = counter;
        job_pointers.push_back(job.handle);
    }

    pending_jobs->enqueue_bulk(job_pointers.begin(), job_pointers.size());
    if (counter)
        counter->count.fetch_add(jobs.size(), std::memory_order_release);
}

void jobs::Scheduler::dispatch_job(JobBase job, Counter* counter)
{
    dispatch_jobs(std::span{&job, 1}, counter);
}

void jobs::Scheduler::worker_thread_loop(const std::stop_token& token, JobQueue pending_jobs)
{
    while (!token.stop_requested())
    {
        JobBase::handle_type handle = nullptr;
        pending_jobs->wait_dequeue(handle);
        if (handle)
        {
            handle.promise().add_switch_information(SwitchType::Resume);
            handle.resume();
        }

    }

    LOGGER_TRACE("[worker_thread] Thread {} stopped", std::this_thread::get_id());
}
} // portal
