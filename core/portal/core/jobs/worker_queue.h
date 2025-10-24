//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//
#pragma once
#include <random>
#include <span>
#include <concurrentqueue/concurrentqueue.h>

#include "job.h"
#include "portal/core/debug/assert.h"

namespace portal
{

enum class JobPriority: uint8_t
{
    Low    = 0,
    Normal = 1,
    High   = 2,
};

template <size_t N = 3> // Should be the same as JobPriority
struct QueueSet
{
    using ItemType = JobBase::handle_type;
    using QueueType = moodycamel::ConcurrentQueue<ItemType>;

    std::array<QueueType, N> queues;

    explicit QueueSet(size_t capacity = 4096)
    {
        std::ranges::for_each(
            queues,
            [capacity](auto& q)
            {
                q = QueueType(capacity);
            }
            );
    }

    //TODO: support token operations?
    bool enqueue(JobPriority priority, const ItemType& item)
    {
        const auto prio_int = static_cast<uint8_t>(priority);
        PORTAL_ASSERT(prio_int < N, "Priority must be in the range of exciting queues");

        return queues[prio_int].enqueue(item);
    }

    template <typename It>
    bool enqueue_bulk(JobPriority priority, It first, size_t size)
    {
        const auto prio_int = static_cast<uint8_t>(priority);
        PORTAL_ASSERT(prio_int < N, "Priority must be in the range of exciting queues");

        return queues[prio_int].enqueue_bulk(first, size);
    }

    bool try_dequeue(JobPriority priority, ItemType& item)
    {
        const auto prio_int = static_cast<uint8_t>(priority);
        PORTAL_ASSERT(prio_int < N, "Priority must be in the range of exciting queues");

        return queues[prio_int].try_dequeue(item);
    }

    template <typename It>
    size_t try_dequeue_bulk(JobPriority priority, It first, size_t size)
    {
        const auto prio_int = static_cast<uint8_t>(priority);
        PORTAL_ASSERT(prio_int < N, "Priority must be in the range of exciting queues");

        return queues[prio_int].try_dequeue_bulk(first, size);
    }
};

class WorkerQueue
{
public:
    void submit_job(JobBase::handle_type& job, JobPriority priority);
    void submit_job_batch(std::span<JobBase::handle_type> jobs, JobPriority priority);

    std::optional<JobBase::handle_type> try_pop();
    size_t try_pop_bulk(JobBase::handle_type* jobs, size_t max_count);

    void migrate_jobs_to_stealable();

    size_t attempt_steal(JobBase::handle_type* jobs, size_t max_count);

    std::array<std::atomic<size_t>, 3>& get_local_count() { return local_count; }
    std::array<std::atomic<size_t>, 3>& get_stealable_count() { return stealable_count; }

private:
    // TODO: have scsp queue here
    QueueSet<> local_set;
    QueueSet<> stealable_set;

    std::array<std::atomic<size_t>, 3> local_count = {0, 0, 0};
    std::array<std::atomic<size_t>, 3> stealable_count = {0, 0, 0};
};

} // portal
