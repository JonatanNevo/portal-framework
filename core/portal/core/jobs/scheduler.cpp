//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "scheduler.h"

namespace portal::jobs
{
static auto logger = Log::get_logger("Scheduler");
thread_local size_t Scheduler::tls_worker_id = std::numeric_limits<size_t>::max();


Scheduler::Scheduler(const int32_t num_worker_threads, const size_t job_cache_size)
    : stats(num_worker_threads)
{
    PORTAL_PROF_ZONE();
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
    for (auto& context : contexts)
    {
        context.job_cache.resize(job_cache_size);
    }
    global_context.job_cache.resize(job_cache_size);

    threads.reserve(num_workers);
    for (size_t i = 0; i < num_workers; ++i)
    {
        threads.emplace_back(
            ThreadSpecification{
                .name = std::format("Worker Thread {}", i),
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


Scheduler::~Scheduler()
{
    PORTAL_PROF_ZONE();
    if (!threads.empty())
    {
        for (auto& thread : threads)
        {
            thread.request_stop();
        }

        for (auto& thread : threads)
        {
            thread.join();
        }
    }
}


void Scheduler::wait_for_counter(Counter& counter)
{
    PORTAL_PROF_ZONE();
    auto& context = get_context();
    while (counter.count.load(std::memory_order_acquire) > 0)
    {
        const auto state = worker_thread_iteration(context);

        if (state == WorkerIterationState::EmptyQueue)
        {
            PORTAL_PROF_ZONE("WaitCounterChange");
#if ENABLE_JOB_STATS
            const auto idle_start = std::chrono::high_resolution_clock::now();
#endif
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
#if ENABLE_JOB_STATS
            const auto idle_end = std::chrono::high_resolution_clock::now();
            const auto idle_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(idle_end - idle_start);
            stats.record_idle_time(tls_worker_id, idle_duration.count());
#endif
        }
    }
}

void Scheduler::wait_for_jobs(const std::span<JobBase> jobs, const JobPriority priority)
{
    PORTAL_PROF_ZONE();
    Counter counter{};

    dispatch_jobs(jobs, priority, &counter);
    wait_for_counter(counter);
}

void Scheduler::dispatch_jobs(const std::span<JobBase> jobs, const JobPriority priority, Counter* counter)
{
    PORTAL_PROF_ZONE();
    llvm::SmallVector<JobBase::handle_type> job_pointers;
    job_pointers.reserve(jobs.size());

    for (auto& job : jobs)
    {
        job.set_dispatched();
        job.set_scheduler(this);
        if (counter)
            job.set_counter(counter);
        job_pointers.push_back(job.handle);
    }

    auto& context = get_context();
    context.queue.submit_job_batch(job_pointers, priority);
    stats.record_work_submitted(tls_worker_id, priority, jobs.size());

    if (counter)
        counter->count.fetch_add(jobs.size(), std::memory_order_release);
}

void Scheduler::dispatch_job(JobBase job, const JobPriority priority, Counter* counter)
{
    PORTAL_PROF_ZONE();
    dispatch_jobs(std::span{&job, 1}, priority, counter);
}

WorkerIterationState Scheduler::main_thread_do_work()
{
    return worker_thread_iteration(global_context);
}

void Scheduler::worker_thread_loop(const std::stop_token& token, size_t worker_id)
{
    PORTAL_PROF_ZONE();
    tls_worker_id = worker_id;
    auto& context = contexts[worker_id];

    while (!token.stop_requested())
    {
        const auto state = worker_thread_iteration(context);

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

    LOGGER_TRACE("[worker_thread] Thread {} stopped", worker_id);
}

WorkerIterationState Scheduler::worker_thread_iteration(WorkerContext& context)
{
    if (context.cache_index > 0)
    {
        execute_job(context.job_cache[--context.cache_index]);
        return WorkerIterationState::Executed;
    }

    if (tls_worker_id < num_workers)
    {
        if (++context.iterations_since_steal_check >= WorkerContext::STEAL_CHECK_INTERVAL)
        {
            context.queue.migrate_jobs_to_stealable();
            context.iterations_since_steal_check = 0;
        }

        const size_t dequeued = context.queue.try_pop_bulk(context.job_cache.data(), context.job_cache.size());
        if (dequeued > 0)
        {
            context.cache_index = dequeued;
            stats.record_queue_hit(tls_worker_id, JobStats::QueueType::Local);
            return WorkerIterationState::FilledCache;
        }
    }

#if ENABLE_JOB_STATS
    if (++context.iterations_since_sample >= WorkerContext::SAMPLE_INTERVAL)
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
        context.iterations_since_sample = 0;
    }
#endif

    const size_t stolen = try_steal(context.job_cache.data(), context.job_cache.size());
    if (stolen > 0)
    {
        context.cache_index = stolen;
        stats.record_queue_hit(tls_worker_id, JobStats::QueueType::Stealable);
        return WorkerIterationState::FilledCache;
    }

    const size_t dequeued_global = try_dequeue_global(context.job_cache.data(), context.job_cache.size());
    if (dequeued_global > 0)
    {
        context.cache_index = dequeued_global;
        stats.record_queue_hit(tls_worker_id, JobStats::QueueType::Global);
        return WorkerIterationState::FilledCache;
    }

    // If we reach this place, it means that there is no work to do
    return WorkerIterationState::EmptyQueue;
}

size_t Scheduler::try_steal(JobBase::handle_type* jobs, const size_t max_size)
{
    if (num_workers == 0)
        return 0;

    auto& context = get_context();
    const size_t victim_id = context.rng() % num_workers;
    if (victim_id == tls_worker_id)
        return 0;

    const auto stolen = contexts[victim_id].queue.attempt_steal(jobs, max_size);
#if ENABLE_JOB_STATS
    if (stolen > 0)
    {
        stats.record_steal_attempt(tls_worker_id, true, stolen);
        stats.record_work_stolen_from_me(victim_id, stolen);
    }
    else
    {
        stats.record_steal_attempt(tls_worker_id, false, 0);
    }
#endif

    return stolen;
}

BasicCoroutine Scheduler::execute_job(const JobBase::handle_type& job)
{
    PORTAL_PROF_ZONE();
#if ENABLE_JOB_STATS
    const auto start = std::chrono::high_resolution_clock::now();
#endif

    if (job)
    {
        job.promise().add_switch_information(SwitchType::Resume);
        co_await job.promise();
    }

#if ENABLE_JOB_STATS
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    stats.record_work_executed(tls_worker_id, duration.count());
#endif
}

Scheduler::WorkerContext& Scheduler::get_context()
{
    if (tls_worker_id < num_workers)
        return contexts.at(tls_worker_id);

    return global_context;
}

size_t Scheduler::try_dequeue_global(JobBase::handle_type* jobs, const size_t max_count)
{
    return global_context.queue.try_pop_bulk(jobs, max_count);
}
} // portal
