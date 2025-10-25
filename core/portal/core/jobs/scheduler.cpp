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

thread_local size_t jobs::Scheduler::tls_worker_id = std::numeric_limits<size_t>::max();

jobs::Scheduler::Scheduler(const int32_t num_worker_threads)
    : global_jobs(std::make_shared<QueueSet<>>()), stats(num_worker_threads)
{
    PORTAL_ASSERT(num_worker_threads >= 0, "Number of worker threads cannot be negative");

    if (num_worker_threads < 0)
    {
        // If negative, then this means that we must count backwards from the number of available hardware threads
        num_workers = static_cast<int32_t>(std::thread::hardware_concurrency()) + num_worker_threads;
    }
    else
    {
        num_workers = num_worker_threads;
    }

    LOGGER_INFO("Initializing scheduler with {} worker threads", num_worker_threads);

    contexts = std::vector<WorkerContext>(num_workers);
    threads.reserve(num_workers);
    for (size_t i = 0; i < num_workers; ++i)
    {
        threads.emplace_back(
            ThreadSpecification{
                .name = fmt::format("Worker Thread {}", i),
                .affinity = ThreadAffinity::CoreLean,
                .core = static_cast<uint16_t>(i)
            },
            [&, index = i](const std::stop_token& stop_token)
            {
                worker_thread_loop(stop_token, index);
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


void jobs::Scheduler::wait_for_jobs(const std::span<JobBase> jobs, const JobPriority priority)
{
    Counter counter{};

    dispatch_jobs(jobs, priority, &counter);

    while (counter.count.load(std::memory_order_acquire) > 0)
    {
        const auto state = worker_thread_iteration();

#if ENABLE_JOB_STATS
        const auto idle_start = std::chrono::high_resolution_clock::now();
#endif
        if (state == WorkerIterationState::EmptyQueue)
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
        }
#if ENABLE_JOB_STATS
        const auto idle_end = std::chrono::high_resolution_clock::now();
        const auto idle_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(idle_end - idle_start);
        stats.record_idle_time(tls_worker_id, idle_duration.count());
#endif
    }
}

void jobs::Scheduler::dispatch_jobs(const std::span<JobBase> jobs, const JobPriority priority, Counter* counter)
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

    if (tls_worker_id < num_workers)
    {
        auto& context = contexts.at(tls_worker_id);
        context.queue.submit_job_batch(job_pointers, priority);
        stats.record_job_submitted(tls_worker_id, priority, jobs.size());
    }
    else
    {
        global_jobs->enqueue_bulk(priority, job_pointers.begin(), job_pointers.size());
    }

    if (counter)
        counter->count.fetch_add(jobs.size(), std::memory_order_release);
}

void jobs::Scheduler::dispatch_job(JobBase job, const JobPriority priority, Counter* counter)
{
    dispatch_jobs(std::span{&job, 1}, priority, counter);
}

void jobs::Scheduler::worker_thread_loop(const std::stop_token& token, size_t worker_id)
{
    tls_worker_id = worker_id;

    while (!token.stop_requested())
    {
        const auto state = worker_thread_iteration();

#if ENABLE_JOB_STATS
        const auto idle_start = std::chrono::high_resolution_clock::now();
#endif

        // TODO: sleep on empty queue?
        if (state == WorkerIterationState::EmptyQueue)
            std::this_thread::yield();

#if ENABLE_JOB_STATS
        const auto idle_end = std::chrono::high_resolution_clock::now();
        const auto idle_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(idle_end - idle_start);
        stats.record_idle_time(tls_worker_id, idle_duration.count());
#endif
    }

    LOGGER_TRACE("[worker_thread] Thread {} stopped", std::this_thread::get_id());
}

jobs::Scheduler::WorkerIterationState jobs::Scheduler::worker_thread_iteration()
{
    constexpr size_t CACHE_SIZE = 64;
    thread_local JobBase::handle_type job_cache[CACHE_SIZE];
    thread_local size_t cached_count = 0;

    constexpr uint32_t STEAL_CHECK_INTERVAL = 128;
    thread_local uint32_t iterations_since_steal_check = 0;

    constexpr uint32_t SAMPLE_INTERVAL = 1000;
    thread_local uint32_t iterations_since_sample = 0;

    if (cached_count > 0)
    {
        execute_job(job_cache[--cached_count]);
        return WorkerIterationState::Executed;
    }

    if (tls_worker_id < num_workers)
    {
        auto& context = contexts.at(tls_worker_id);

        const size_t dequeued = context.queue.try_pop_bulk(job_cache, CACHE_SIZE);
        if (dequeued > 0)
        {
            cached_count = dequeued;
            stats.record_queue_hit(tls_worker_id, JobStats::QueueType::Local);
            return WorkerIterationState::FilledCache;
        }

        if (++iterations_since_steal_check >= STEAL_CHECK_INTERVAL)
        {
            context.queue.migrate_jobs_to_stealable();
            iterations_since_steal_check = 0;
        }

#if ENABLE_JOB_STATS
        if (++iterations_since_sample >= SAMPLE_INTERVAL)
        {
            const size_t local_depth =
                context.queue.get_local_count()[0].load(std::memory_order_relaxed) +
                context.queue.get_local_count()[1].load(std::memory_order_relaxed) +
                context.queue.get_local_count()[2].load(std::memory_order_relaxed);

            const size_t stealable_depth =
                context.queue.get_stealable_count()[0].load(std::memory_order_relaxed) +
                context.queue.get_stealable_count()[1].load(std::memory_order_relaxed) +
                context.queue.get_stealable_count()[2].load(std::memory_order_relaxed);

            stats.record_queue_depth(tls_worker_id, local_depth, stealable_depth);
            iterations_since_sample = 0;
        }
#endif

        const size_t stolen = try_steal(job_cache, CACHE_SIZE);
        if (stolen > 0)
        {
            cached_count = stolen;
            stats.record_queue_hit(tls_worker_id, JobStats::QueueType::Stealable);
            return WorkerIterationState::FilledCache;
        }
    }

    const size_t dequeued_global = try_dequeue_global(job_cache, CACHE_SIZE);
    if (dequeued_global > 0)
    {
        cached_count = dequeued_global;
        stats.record_queue_hit(tls_worker_id, JobStats::QueueType::Global);
        return WorkerIterationState::FilledCache;
    }

    // If we reach this place, it means that there is no work to do
    return WorkerIterationState::EmptyQueue;
}

size_t jobs::Scheduler::try_steal(JobBase::handle_type* jobs, size_t max_size)
{
    auto& context = contexts[tls_worker_id];

    // Allows self stealing, if we reach this part it means that we are out of local work
    const size_t victim_id = context.rng() % num_workers;
    if (victim_id == tls_worker_id)
        return 0;

    const auto stolen = contexts[victim_id].queue.attempt_steal(jobs, max_size);
#if ENABLE_JOB_STATS
    if (stolen > 0)
    {
        stats.record_steal_attempt(tls_worker_id, true, stolen);
        stats.record_job_stolen_from_me(victim_id, stolen);
    }
    else
    {
        stats.record_steal_attempt(tls_worker_id, false, 0);
    }
#endif

    return stolen;
}

void jobs::Scheduler::execute_job(const JobBase::handle_type& job)
{
#if ENABLE_JOB_STATS
    const auto start = std::chrono::high_resolution_clock::now();
#endif

    if (job)
    {
        job.promise().add_switch_information(SwitchType::Resume);
        job.resume();
    }

#if ENABLE_JOB_STATS
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    stats.record_job_executed(tls_worker_id, duration.count());
#endif
}

size_t jobs::Scheduler::try_dequeue_global(JobBase::handle_type* jobs, const size_t max_count) const
{
    size_t total = 0;
    const size_t high_count = global_jobs->try_dequeue_bulk(JobPriority::High, jobs, max_count);
    total += high_count;

    if (total < max_count)
    {
        const size_t normal_count = global_jobs->try_dequeue_bulk(JobPriority::Normal, jobs + total, max_count - total);
        total += normal_count;
    }

    if (total < max_count)
    {
        const size_t low_count = global_jobs->try_dequeue_bulk(JobPriority::Low, jobs + total, max_count - total);
        total += low_count;
    }

    return total;
}
} // portal
