//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//
#include "worker_queue.h"

#include "portal/core/debug/profile.h"

namespace portal
{
void WorkerQueue::submit_job(JobBase::handle_type& job, JobPriority priority)
{
    const auto priority_num = static_cast<uint8_t>(priority);

    // TODO: should this be enqueue or try_enqueue?
    local_set.enqueue(priority, job);
    local_count[priority_num].fetch_add(1, std::memory_order_relaxed);
}

void WorkerQueue::submit_job_batch(const std::span<JobBase::handle_type> jobs, JobPriority priority)
{
    const auto priority_num = static_cast<uint8_t>(priority);

    size_t enqueued = 0;
    const bool res = local_set.enqueue_bulk(priority, jobs.begin(), jobs.size());
    if (res)
        enqueued += jobs.size();
    local_count[priority_num].fetch_add(enqueued, std::memory_order_relaxed);
}

std::optional<JobBase::handle_type> WorkerQueue::try_pop()
{
    JobBase::handle_type handle;

    if (local_set.try_dequeue(JobPriority::High, handle))
    {
        local_count[static_cast<uint8_t>(JobPriority::High)].fetch_sub(1, std::memory_order_relaxed);
        return handle;
    }

    if (local_set.try_dequeue(JobPriority::Normal, handle))
    {
        local_count[static_cast<uint8_t>(JobPriority::Normal)].fetch_sub(1, std::memory_order_relaxed);
        return handle;
    }

    if (local_set.try_dequeue(JobPriority::Low, handle))
    {
        local_count[static_cast<uint8_t>(JobPriority::Low)].fetch_sub(1, std::memory_order_relaxed);
        return handle;
    }

    return std::nullopt;
}

size_t WorkerQueue::try_pop_bulk(JobBase::handle_type* jobs, size_t max_count)
{
    size_t total = 0;

    const size_t high_count = local_set.try_dequeue_bulk(JobPriority::High, jobs, max_count);
    total += high_count;
    local_count[static_cast<uint8_t>(JobPriority::High)].fetch_sub(high_count, std::memory_order_relaxed);

    if (total < max_count)
    {
        const size_t normal_count = local_set.try_dequeue_bulk(JobPriority::Normal, jobs + total, max_count - total);
        total += normal_count;
        local_count[static_cast<uint8_t>(JobPriority::Normal)].fetch_sub(normal_count, std::memory_order_relaxed);
    }

    if (total < max_count)
    {
        const size_t low_count = local_set.try_dequeue_bulk(JobPriority::Low, jobs + total, max_count - total);
        total += low_count;
        local_count[static_cast<uint8_t>(JobPriority::Low)].fetch_sub(low_count, std::memory_order_relaxed);
    }

    return total;
}

void WorkerQueue::migrate_jobs_to_stealable()
{
    constexpr size_t THRESHOLD = 64;  // Keep some work local
    constexpr size_t MOVE_COUNT = 32; // Move this many at a time

    auto move_to_stealable = [&](
        const JobPriority priority,
        QueueSet<>& local_queue,
        QueueSet<>& stealable_queue,
        std::atomic<size_t>& local_cnt,
        std::atomic<size_t>& stealable_cnt
    )
    {
        if (local_cnt.load(std::memory_order_relaxed) > THRESHOLD)
        {
            std::array<JobBase::handle_type, MOVE_COUNT> jobs;
            const size_t count = local_queue.try_dequeue_bulk(priority, jobs.data(), MOVE_COUNT);

            if (count > 0)
            {
                const bool res = stealable_queue.enqueue_bulk(
                    priority,
                    jobs.begin(),
                    count
                );

                local_cnt.fetch_sub(count, std::memory_order_relaxed);
                if (res)
                    stealable_cnt.fetch_add(count, std::memory_order_release);
            }
        }
    };

    move_to_stealable(
        JobPriority::High,
        local_set,
        stealable_set,
        local_count[static_cast<uint8_t>(JobPriority::High)],
        stealable_count[static_cast<uint8_t>(JobPriority::High)]
    );

    move_to_stealable(
        JobPriority::Normal,
        local_set,
        stealable_set,
        local_count[static_cast<uint8_t>(JobPriority::Normal)],
        stealable_count[static_cast<uint8_t>(JobPriority::Normal)]
    );

    move_to_stealable(
        JobPriority::Low,
        local_set,
        stealable_set,
        local_count[static_cast<uint8_t>(JobPriority::Low)],
        stealable_count[static_cast<uint8_t>(JobPriority::Low)]
    );
}

size_t WorkerQueue::attempt_steal(JobBase::handle_type* jobs, const size_t max_count)
{
    PORTAL_PROF_ZONE();
    size_t total_stolen = 0;

    if (stealable_count[static_cast<uint8_t>(JobPriority::High)].load(std::memory_order_acquire) > 0)
    {
        const size_t stolen = stealable_set.try_dequeue_bulk(JobPriority::High, jobs, max_count);
        if (stolen > 0)
        {
            stealable_count[static_cast<uint8_t>(JobPriority::High)].fetch_sub(stolen, std::memory_order_relaxed);
            total_stolen += stolen;
            if (stolen >= max_count)
                return total_stolen;
        }
    }

    if (stealable_count[static_cast<uint8_t>(JobPriority::Normal)].load(std::memory_order_acquire) > 0)
    {
        const size_t stolen = stealable_set.try_dequeue_bulk(JobPriority::Normal, jobs + total_stolen, max_count - total_stolen);
        if (stolen > 0)
        {
            stealable_count[static_cast<uint8_t>(JobPriority::Normal)].fetch_sub(stolen, std::memory_order_relaxed);
            total_stolen += stolen;
            if (stolen >= max_count)
                return total_stolen;
        }
    }

    if (stealable_count[static_cast<uint8_t>(JobPriority::Low)].load(std::memory_order_acquire) > 0)
    {
        const size_t stolen = stealable_set.try_dequeue_bulk(JobPriority::Low, jobs + total_stolen, max_count - total_stolen);
        if (stolen > 0)
        {
            stealable_count[static_cast<uint8_t>(JobPriority::Low)].fetch_sub(stolen, std::memory_order_relaxed);
            total_stolen += stolen;
            if (stolen >= max_count)
                return total_stolen;
        }
    }

    return total_stolen;
}
} // portal
