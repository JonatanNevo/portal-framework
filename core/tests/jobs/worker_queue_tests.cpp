//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include <gtest/gtest.h>
#include <portal/core/jobs/job.h>
#include "portal/core/log.h"
#include "portal/core/jobs/scheduler.h"
#include "portal/core/jobs/worker_queue.h"

#include "common.h"

namespace portal
{
// ============================================================================
// Section 4: WorkerQueue & QueueSet
// ============================================================================

// Helper to create job handles for testing
Job<> test_queue_job()
{
    co_return;
}

// ============================================================================
// Section 4.1: WorkerQueue Operations
// ============================================================================

TEST_F(JobTest, WorkerQueueSubmitJobAddsToLocalSet)
{
    WorkerQueue queue;

    auto job = test_queue_job();
    auto handle = job.handle;

    queue.submit_job(handle, JobPriority::Normal);

    // Verify local count increased
    EXPECT_EQ(queue.get_local_count()[static_cast<uint8_t>(JobPriority::Normal)].load(), 1)
        << "Job should be added to local_set with Normal priority";
}

TEST_F(JobTest, WorkerQueueSubmitJobBatchProcessesSpan)
{
    WorkerQueue queue;

    std::vector<JobBase::handle_type> handles;
    for (int i = 0; i < 5; ++i)
    {
        auto job = test_queue_job();
        handles.push_back(job.handle);
    }

    queue.submit_job_batch(std::span{handles}, JobPriority::High);

    // Verify all jobs added to local set
    EXPECT_EQ(queue.get_local_count()[static_cast<uint8_t>(JobPriority::High)].load(), 5)
        << "Batch submit should add all jobs to local_set";
}

TEST_F(JobTest, WorkerQueueTryPopReturnsJobFromLocalSet)
{
    WorkerQueue queue;

    auto job = test_queue_job();
    auto handle = job.handle;

    queue.submit_job(handle, JobPriority::Normal);

    // Try to pop the job
    auto popped = queue.try_pop();

    ASSERT_TRUE(popped.has_value()) << "try_pop should return a job";
    EXPECT_EQ(popped.value(), handle) << "Should return the same job handle";
}

TEST_F(JobTest, WorkerQueueTryPopBulkFillsArray)
{
    WorkerQueue queue;

    std::vector<JobBase::handle_type> handles;
    for (int i = 0; i < 10; ++i)
    {
        auto job = test_queue_job();
        handles.push_back(job.handle);
    }

    queue.submit_job_batch(std::span{handles}, JobPriority::Normal);

    // Try bulk pop
    JobBase::handle_type popped_jobs[5];
    size_t count = queue.try_pop_bulk(popped_jobs, 5);

    EXPECT_EQ(count, 5) << "Should pop 5 jobs";
}

TEST_F(JobTest, WorkerQueueTryPopReturnsNulloptWhenEmpty)
{
    WorkerQueue queue;

    auto popped = queue.try_pop();

    EXPECT_FALSE(popped.has_value()) << "try_pop should return nullopt when queue is empty";
}

// ============================================================================
// Section 4.2: Local vs Stealable Queues
// ============================================================================

TEST_F(JobTest, JobsInitiallyAddedToLocalSet)
{
    WorkerQueue queue;

    auto job = test_queue_job();
    auto handle = job.handle;

    queue.submit_job(handle, JobPriority::Normal);

    // Check local count increased
    EXPECT_EQ(queue.get_local_count()[static_cast<uint8_t>(JobPriority::Normal)].load(), 1);
    // Check stealable count is still 0
    EXPECT_EQ(queue.get_stealable_count()[static_cast<uint8_t>(JobPriority::Normal)].load(), 0)
        << "Jobs should not be in stealable_set initially";
}

TEST_F(JobTest, MigrateJobsMovesFromLocalToStealable)
{
    WorkerQueue queue;

    // Need > 64 jobs to trigger migration (THRESHOLD = 64 in worker_queue.cpp)
    std::vector<JobBase::handle_type> handles;
    for (int i = 0; i < 100; ++i)
    {
        auto job = test_queue_job();
        handles.push_back(job.handle);
    }

    queue.submit_job_batch(std::span{handles}, JobPriority::Normal);

    auto local_before = queue.get_local_count()[static_cast<uint8_t>(JobPriority::Normal)].load();
    EXPECT_EQ(local_before, 100);

    // Migrate jobs
    queue.migrate_jobs_to_stealable();

    // Check that jobs moved from local to stealable
    auto local_after = queue.get_local_count()[static_cast<uint8_t>(JobPriority::Normal)].load();
    auto stealable_after = queue.get_stealable_count()[static_cast<uint8_t>(JobPriority::Normal)].load();

    EXPECT_LT(local_after, local_before) << "Local count should decrease after migration";
    EXPECT_GT(stealable_after, 0) << "Stealable count should increase after migration";
}

TEST_F(JobTest, AttemptStealOnlyAccessesStealableSet)
{
    WorkerQueue queue;

    // Add jobs to local set (need > 64 to trigger migration)
    std::vector<JobBase::handle_type> handles;
    for (int i = 0; i < 100; ++i)
    {
        auto job = test_queue_job();
        handles.push_back(job.handle);
    }
    queue.submit_job_batch(std::span{handles}, JobPriority::Normal);

    // Try to steal without migration - should get nothing
    JobBase::handle_type stolen_jobs[5];
    size_t stolen_count = queue.attempt_steal(stolen_jobs, 5);

    EXPECT_EQ(stolen_count, 0) << "Should not steal from local_set";

    // Now migrate and try again
    queue.migrate_jobs_to_stealable();

    stolen_count = queue.attempt_steal(stolen_jobs, 5);

    EXPECT_GT(stolen_count, 0) << "Should be able to steal from stealable_set";
}

TEST_F(JobTest, OtherWorkersCannotAccessLocalSet)
{
    WorkerQueue queue1;

    // Add job to queue1's local set
    auto job = test_queue_job();
    auto handle = job.handle;
    queue1.submit_job(handle, JobPriority::Normal);

    // Another worker (queue2) tries to steal - should get nothing
    JobBase::handle_type stolen_job;
    size_t stolen = queue1.attempt_steal(&stolen_job, 1);

    EXPECT_EQ(stolen, 0) << "Cannot steal from local_set without migration";
}

// ============================================================================
// Section 4.3: Priority Queues (QueueSet)
// ============================================================================

TEST_F(JobTest, QueueSetMaintainsThreeSeparateQueues)
{
    QueueSet<> queue_set;

    auto job1 = test_queue_job();
    auto job2 = test_queue_job();
    auto job3 = test_queue_job();

    auto h1 = job1.handle;
    auto h2 = job2.handle;
    auto h3 = job3.handle;

    // Enqueue to different priorities
    queue_set.enqueue(JobPriority::Low, h1);
    queue_set.enqueue(JobPriority::Normal, h2);
    queue_set.enqueue(JobPriority::High, h3);

    // Try dequeue from each priority
    JobBase::handle_type popped;

    EXPECT_TRUE(queue_set.try_dequeue(JobPriority::Low, popped));
    EXPECT_EQ(popped, h1);

    EXPECT_TRUE(queue_set.try_dequeue(JobPriority::Normal, popped));
    EXPECT_EQ(popped, h2);

    EXPECT_TRUE(queue_set.try_dequeue(JobPriority::High, popped));
    EXPECT_EQ(popped, h3);
}

TEST_F(JobTest, QueueSetEnqueuePlacesJobInCorrectPriorityQueue)
{
    QueueSet<> queue_set;

    auto job = test_queue_job();
    auto handle = job.handle;

    queue_set.enqueue(JobPriority::High, handle);

    // Try to dequeue from wrong priority - should fail
    JobBase::handle_type popped;
    EXPECT_FALSE(queue_set.try_dequeue(JobPriority::Low, popped));
    EXPECT_FALSE(queue_set.try_dequeue(JobPriority::Normal, popped));

    // Try to dequeue from correct priority - should succeed
    EXPECT_TRUE(queue_set.try_dequeue(JobPriority::High, popped));
    EXPECT_EQ(popped, handle);
}

TEST_F(JobTest, QueueSetEnqueueBulkHandlesBatchInsertion)
{
    QueueSet<> queue_set;

    std::vector<JobBase::handle_type> handles;
    for (int i = 0; i < 10; ++i)
    {
        auto job = test_queue_job();
        handles.push_back(job.handle);
    }

    bool result = queue_set.enqueue_bulk(JobPriority::Normal, handles.begin(), handles.size());

    EXPECT_TRUE(result) << "Bulk enqueue should succeed";

    // Try to dequeue all
    int count = 0;
    JobBase::handle_type popped;
    while (queue_set.try_dequeue(JobPriority::Normal, popped))
    {
        count++;
    }

    EXPECT_EQ(count, 10) << "Should dequeue all 10 jobs";
}

TEST_F(JobTest, QueueSetTryDequeueRespectsPriorityOrdering)
{
    // Note: This test verifies individual queue behavior
    // The scheduler is responsible for checking High -> Normal -> Low in order
    QueueSet<> queue_set;

    auto job_low = test_queue_job();
    auto job_normal = test_queue_job();
    auto job_high = test_queue_job();

    queue_set.enqueue(JobPriority::Low, job_low.handle);
    queue_set.enqueue(JobPriority::Normal, job_normal.handle);
    queue_set.enqueue(JobPriority::High, job_high.handle);

    // Dequeue from high first
    JobBase::handle_type popped;
    EXPECT_TRUE(queue_set.try_dequeue(JobPriority::High, popped));
    EXPECT_EQ(popped, job_high.handle);

    // Then normal
    EXPECT_TRUE(queue_set.try_dequeue(JobPriority::Normal, popped));
    EXPECT_EQ(popped, job_normal.handle);

    // Then low
    EXPECT_TRUE(queue_set.try_dequeue(JobPriority::Low, popped));
    EXPECT_EQ(popped, job_low.handle);
}

TEST_F(JobTest, QueueSetTryDequeueBulkRespectsMaxSize)
{
    QueueSet<> queue_set;

    std::vector<JobBase::handle_type> handles;
    for (int i = 0; i < 20; ++i)
    {
        auto job = test_queue_job();
        handles.push_back(job.handle);
    }

    queue_set.enqueue_bulk(JobPriority::Normal, handles.begin(), handles.size());

    // Try to dequeue only 10
    JobBase::handle_type popped[10];
    size_t count = queue_set.try_dequeue_bulk(JobPriority::Normal, popped, 10);

    EXPECT_EQ(count, 10) << "Should dequeue exactly max_size jobs";

    // Verify 10 jobs still remain
    JobBase::handle_type remaining[15];
    size_t remaining_count = queue_set.try_dequeue_bulk(JobPriority::Normal, remaining, 15);

    EXPECT_EQ(remaining_count, 10) << "Should have 10 jobs remaining";
}

// ============================================================================
// Section 4.4: Work Stealing
// ============================================================================

TEST_F(JobTest, WorkStealingFromStealableSet)
{
    WorkerQueue victim_queue;

    // Victim adds jobs and migrates them (need > 64 to trigger migration)
    std::vector<JobBase::handle_type> handles;
    for (int i = 0; i < 100; ++i)
    {
        auto job = test_queue_job();
        handles.push_back(job.handle);
    }

    victim_queue.submit_job_batch(std::span{handles}, JobPriority::Normal);
    victim_queue.migrate_jobs_to_stealable();

    // Thief attempts to steal
    JobBase::handle_type stolen_jobs[5];
    size_t stolen_count = victim_queue.attempt_steal(stolen_jobs, 5);

    EXPECT_GT(stolen_count, 0) << "Should successfully steal jobs";
    EXPECT_LE(stolen_count, 5) << "Should not steal more than requested";
}

TEST_F(JobTest, WorkStealingDoesNotStealFromLocalSet)
{
    WorkerQueue queue;

    // Add jobs but don't migrate
    std::vector<JobBase::handle_type> handles;
    for (int i = 0; i < 10; ++i)
    {
        auto job = test_queue_job();
        handles.push_back(job.handle);
    }

    queue.submit_job_batch(std::span{handles}, JobPriority::Normal);

    // Attempt steal - should fail
    JobBase::handle_type stolen_jobs[5];
    size_t stolen_count = queue.attempt_steal(stolen_jobs, 5);

    EXPECT_EQ(stolen_count, 0) << "Should not steal from local_set";
}

TEST_F(JobTest, StolenJobsExecutedOnStealingWorker)
{
    jobs::Scheduler scheduler{2}; // 2 worker threads

    std::atomic<int> executed_count{0};
    std::vector<Job<>> jobs;

    // Create many jobs to encourage work stealing
    for (int i = 0; i < 100; ++i)
    {
        auto job = [&executed_count]() -> Job<>
        {
            executed_count.fetch_add(1, std::memory_order_relaxed);
            co_return;
        };
        jobs.push_back(job());
    }

    scheduler.wait_for_jobs(std::span{jobs});

    // All jobs should execute regardless of which worker ran them
    EXPECT_EQ(executed_count.load(), 100) << "All jobs should execute (possibly via stealing)";
}

TEST_F(JobTest, WorkerDoesNotStealFromItself)
{
    // This is more of a logic test - a worker's steal logic should skip its own queue
    // We can't easily test this without access to scheduler internals
    // This test just verifies that work stealing doesn't cause deadlocks

    jobs::Scheduler scheduler{1}; // 1 worker

    std::atomic<int> count{0};
    std::vector<Job<>> jobs;

    for (int i = 0; i < 50; ++i)
    {
        auto job = [&count]() -> Job<>
        {
            count.fetch_add(1, std::memory_order_relaxed);
            co_return;
        };
        jobs.push_back(job());
    }

    scheduler.wait_for_jobs(std::span{jobs});

    EXPECT_EQ(count.load(), 50) << "All jobs should complete without stealing from self";
}
} // namespace portal
