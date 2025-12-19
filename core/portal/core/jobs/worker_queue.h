//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//
#pragma once
#include <random>
#include <span>
#include <concurrentqueue/moodycamel/concurrentqueue.h>


#include "job.h"
#include "portal/core/debug/assert.h"

namespace portal
{
/**
 * Job execution priority levels.
 *
 * High-priority jobs are dequeued before normal and low-priority jobs.
 */
enum class JobPriority: uint8_t
{
    Low    = 0,
    Normal = 1,
    High   = 2,
};

/**
 * Set of concurrent queues indexed by priority level.
 *
 * @tparam N Number of priority levels (defaults to 3 for Low/Normal/High)
 */
template <size_t N = 3> // Should be the same as JobPriority
struct QueueSet
{
    using ItemType = JobBase::handle_type;
    using QueueType = moodycamel::ConcurrentQueue<ItemType>;

    std::array<QueueType, N> queues;

    explicit QueueSet(size_t capacity = 4096)
    {
        for (auto& q : queues)
            q = QueueType(capacity);
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

/**
 * Per-worker job queue with local and stealable queues.
 *
 * Workers submit jobs to their local queue. Jobs are migrated to the stealable queue
 * periodically, allowing idle workers to steal them for load balancing.
 */
class WorkerQueue
{
public:
    /**
     * Submit a single job to the local queue.
     *
     * @param job Job handle to enqueue
     * @param priority Job priority level
     */
    void submit_job(JobBase::handle_type& job, JobPriority priority);

    /**
     * Submit multiple jobs to the local queue.
     *
     * @param jobs Span of job handles to enqueue
     * @param priority Job priority level
     */
    void submit_job_batch(std::span<JobBase::handle_type> jobs, JobPriority priority);

    /**
     * Try to pop a job from the local queue (highest priority first).
     *
     * @return Job handle if available, nullopt otherwise
     */
    std::optional<JobBase::handle_type> try_pop();

    /**
     * Try to pop multiple jobs from the local queue.
     *
     * @param jobs Output buffer for job handles
     * @param max_count Maximum number of jobs to pop
     * @return Number of jobs actually popped
     */
    size_t try_pop_bulk(JobBase::handle_type* jobs, size_t max_count);

    /**
     * Move jobs from local queue to stealable queue.
     *
     * Called periodically to make jobs available for work stealing.
     */
    void migrate_jobs_to_stealable();

    /**
     * Attempt to steal jobs from the stealable queue.
     *
     * @param jobs Output buffer for stolen job handles
     * @param max_count Maximum number of jobs to steal
     * @return Number of jobs actually stolen
     */
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
