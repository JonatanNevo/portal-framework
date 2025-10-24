//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#pragma once

#include <array>
#include <chrono>
#include <limits>
#include <mutex>

#include "portal/core/jobs/worker_queue.h"

#ifndef ENABLE_JOB_STATS
#define ENABLE_JOB_STATS 1
#endif

namespace portal
{

class JobStats
{
public:
    enum class QueueType
    {
        Local,
        Stealable,
        Global,
    };

    // Per-thread statistics (no contention)
    struct ThreadStats
    {
        // Job execution
        size_t job_executed = 0;
        size_t job_submitted = 0;
        size_t total_job_time_ns = 0;
        size_t min_job_time_ns = std::numeric_limits<size_t>::max();
        size_t max_job_time_ns = 0;

        // Per-priority job counts (submitted)
        std::array<size_t, 3> jobs_by_priority = {0, 0, 0};

        // Work stealing
        size_t steal_attempts = 0;
        size_t steal_successes = 0;
        size_t jobs_stolen = 0;
        size_t jobs_lost_to_thieves = 0;

        // Queue depths (sampled periodically)
        size_t total_queue_depth_samples = 0;
        size_t sum_local_queue_depth = 0;
        size_t sum_stealable_queue_depth = 0;
        size_t max_local_queue_depth = 0;
        size_t max_stealable_queue_depth = 0;

        // Idle
        size_t idle_spins = 0;
        size_t total_idle_time_ns = 0;

        // Cache efficiency
        size_t local_queue_hits = 0;
        size_t steal_queue_hits = 0;
        size_t global_queue_hits = 0;
    };

    // Global aggregated statistics
    struct GlobalStats
    {
        size_t total_jobs_executed = 0;
        size_t total_jobs_submitted = 0;
        size_t total_job_time_ns = 0;
        double average_job_time_us = 0.0;
        size_t min_job_time_ns = std::numeric_limits<size_t>::max();
        size_t max_job_time_ns = 0;

        std::array<size_t, 3> jobs_by_priority = {0, 0, 0};

        size_t total_steal_attempts = 0;
        size_t total_steal_successes = 0;
        double steal_success_rate = 0.0;

        double average_local_queue_depth = 0.0;
        double average_stealable_queue_depth = 0.0;
        size_t max_queue_depth = 0;

        size_t total_idle_spins = 0;
        size_t total_idle_time_ns = 0;
        double idle_time_percentage = 0.0;

        // Coefficient of variation of jobs per thread
        double load_imbalance = 0.0;

        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point last_reset;
        double elapsed_seconds = 0.0;
    };

public:
    explicit JobStats(size_t num_threads);

    // Local per-thread stats
    void record_job_submitted(size_t worker_id, JobPriority priority, size_t count = 1);
    void record_job_executed(size_t worker_id, size_t duration_ns);
    void record_steal_attempt(size_t worker_id, bool success, size_t jobs_stolen);
    void record_job_stolen_from_me(size_t worker_id, size_t count);
    void record_queue_depth(size_t worker_id, size_t local_depth, size_t stealable_depth);
    void record_idle_spin(size_t worker_id);
    void record_idle_time(size_t worker_id, size_t duration_ns);
    void record_queue_hit(size_t worker_id, QueueType type);

    // Aggregate statistics for all threads
    GlobalStats aggregate();

    void reset();

    void log() const;

    const std::vector<ThreadStats>& get_thread_stats() const;
    const GlobalStats& get_global_stats() const;

private:
    std::vector<ThreadStats> thread_stats;
    GlobalStats global_stats{};
    std::chrono::steady_clock::time_point start_time;
    mutable std::mutex stats_mutex;
};

} // portal