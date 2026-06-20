//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "job_stats.h"

namespace portal
{
static auto logger = Log::get_logger("Core");

JobStats::JobStats(const size_t num_threads) : thread_stats(num_threads), start_time(std::chrono::steady_clock::now())
{
    global_stats.start_time = start_time;
    global_stats.last_reset = start_time;
}

void JobStats::record_work_submitted([[maybe_unused]] const size_t worker_id, [[maybe_unused]] JobPriority priority, [[maybe_unused]] size_t count)
{
#if ENABLE_JOB_STATS
    ThreadStats* stats = nullptr;
    if (worker_id < thread_stats.size())
    {
        stats = &thread_stats.at(worker_id);
    }
    else
    {
        stats = &main_stats;
    }

    stats->work_submitted += count;
    stats->work_by_priority[static_cast<uint8_t>(priority)]++;
#endif
}

void JobStats::record_work_executed([[maybe_unused]] const size_t worker_id, [[maybe_unused]] const size_t duration_ns)
{
#if ENABLE_JOB_STATS
    ThreadStats* stats = nullptr;
    if (worker_id < thread_stats.size())
    {
        stats = &thread_stats.at(worker_id);
    }
    else
    {
        stats = &main_stats;
    }

    stats->work_executed++;
    stats->total_work_time_ns += duration_ns;
    stats->min_work_time_ns = std::min(stats->min_work_time_ns, duration_ns);
    stats->max_work_time_ns = std::max(stats->max_work_time_ns, duration_ns);
#endif
}

void JobStats::record_steal_attempt(
    [[maybe_unused]] const size_t worker_id,
    [[maybe_unused]] const bool success,
    [[maybe_unused]] const size_t jobs_stolen
)
{
#if ENABLE_JOB_STATS
    ThreadStats* stats = nullptr;
    if (worker_id < thread_stats.size())
    {
        stats = &thread_stats.at(worker_id);
    }
    else
    {
        stats = &main_stats;
    }

    stats->steal_attempts++;
    if (success)
    {
        stats->steal_successes++;
        stats->work_stolen += jobs_stolen;
    }
#endif
}

void JobStats::record_work_stolen_from_me([[maybe_unused]] const size_t worker_id, [[maybe_unused]] const size_t count)
{
#if ENABLE_JOB_STATS
    auto& stats = thread_stats.at(worker_id);
    stats.work_lost_to_thieves += count;
#endif
}

void JobStats::record_queue_depth(
    [[maybe_unused]] const size_t worker_id,
    [[maybe_unused]] const size_t local_depth,
    [[maybe_unused]] const size_t stealable_depth
)
{
#if ENABLE_JOB_STATS
    ThreadStats* stats = nullptr;
    if (worker_id < thread_stats.size())
    {
        stats = &thread_stats.at(worker_id);
    }
    else
    {
        stats = &main_stats;
    }

    stats->total_queue_depth_samples++;
    stats->sum_local_queue_depth += local_depth;
    stats->sum_stealable_queue_depth += stealable_depth;
    stats->max_local_queue_depth = std::max(stats->max_local_queue_depth, local_depth);
    stats->max_stealable_queue_depth = std::max(stats->max_stealable_queue_depth, stealable_depth);
#endif
}

void JobStats::record_idle_spin([[maybe_unused]] const size_t worker_id)
{
#if ENABLE_JOB_STATS
    ThreadStats* stats = nullptr;
    if (worker_id < thread_stats.size())
    {
        stats = &thread_stats.at(worker_id);
    }
    else
    {
        stats = &main_stats;
    }

    stats->idle_spins++;
#endif
}

void JobStats::record_idle_time([[maybe_unused]] const size_t worker_id, [[maybe_unused]] const size_t duration_ns)
{
#if ENABLE_JOB_STATS
    ThreadStats* stats = nullptr;
    if (worker_id < thread_stats.size())
    {
        stats = &thread_stats.at(worker_id);
    }
    else
    {
        stats = &main_stats;
    }

    stats->total_idle_time_ns += duration_ns;
#endif
}

void JobStats::record_queue_hit([[maybe_unused]] size_t worker_id, [[maybe_unused]] QueueType type)
{
#if ENABLE_JOB_STATS
    ThreadStats* stats = nullptr;
    if (worker_id < thread_stats.size())
    {
        stats = &thread_stats.at(worker_id);
    }
    else
    {
        stats = &main_stats;
    }

    switch (type)
    {
    case QueueType::Local:
        stats->local_queue_hits++;
        break;
    case QueueType::Stealable:
        stats->steal_queue_hits++;
        break;
    case QueueType::Global:
        stats->global_queue_hits++;
        break;
    }
#endif
}

JobStats::GlobalStats JobStats::aggregate()
{
    GlobalStats stats;
#if ENABLE_JOB_STATS
    std::lock_guard lock(stats_mutex);

    stats.start_time = start_time;
    stats.last_reset = global_stats.last_reset;

    const auto now = std::chrono::steady_clock::now();
    stats.elapsed_seconds = std::chrono::duration<double>(now - start_time).count();

    llvm::SmallVector<size_t, 8> work_per_thread;
    work_per_thread.reserve(thread_stats.size());

    for (const auto& thread : thread_stats)
    {
        stats.total_work_executed += thread.work_executed;
        stats.total_work_submitted += thread.work_submitted;
        stats.total_work_time_ns += thread.total_work_time_ns;
        stats.min_work_time_ns = std::min(stats.min_work_time_ns, thread.min_work_time_ns);
        stats.max_work_time_ns = std::max(stats.max_work_time_ns, thread.max_work_time_ns);

        for (size_t i = 0; i < 3; ++i)
        {
            stats.work_by_priority[i] += thread.work_by_priority[i];
        }

        stats.total_steal_attempts += thread.steal_attempts;
        stats.total_steal_successes += thread.steal_successes;

        if (thread.total_queue_depth_samples > 0)
        {
            global_stats.average_local_queue_depth += static_cast<double>(thread.sum_local_queue_depth) / thread.total_queue_depth_samples;
            global_stats.average_stealable_queue_depth += static_cast<double>(thread.sum_stealable_queue_depth) / thread.total_queue_depth_samples;
        }
        stats.max_queue_depth = std::max(
            {
                stats.max_queue_depth,
                thread.max_local_queue_depth,
                thread.max_stealable_queue_depth
            }
        );

        stats.total_idle_spins += thread.idle_spins;
        stats.total_idle_time_ns += thread.total_idle_time_ns;

        work_per_thread.push_back(thread.work_executed);
    }

    stats.total_work_executed += main_stats.work_executed;
    stats.total_work_submitted += main_stats.work_submitted;
    stats.total_work_time_ns += main_stats.total_work_time_ns;
    stats.min_work_time_ns = std::min(stats.min_work_time_ns, main_stats.min_work_time_ns);
    stats.max_work_time_ns = std::max(stats.max_work_time_ns, main_stats.max_work_time_ns);

    for (size_t i = 0; i < 3; ++i)
    {
        stats.work_by_priority[i] += main_stats.work_by_priority[i];
    }

    stats.total_idle_spins += main_stats.idle_spins;
    stats.total_idle_time_ns += main_stats.total_idle_time_ns;

    work_per_thread.push_back(main_stats.work_executed);

    if (stats.total_work_executed > 0)
        stats.average_work_time_us = static_cast<double>(stats.total_work_time_ns) / stats.total_work_executed / 1000.0;

    if (stats.total_steal_attempts > 0)
        stats.steal_success_rate = static_cast<double>(stats.total_steal_successes) / stats.total_steal_attempts * 100.0;

    stats.average_local_queue_depth /= thread_stats.size();
    stats.average_stealable_queue_depth /= thread_stats.size();

    const double total_idle_time_ms = static_cast<double>(stats.total_idle_time_ns) / 1'000'000.0;

    const double total_possible_time_ms = stats.elapsed_seconds * 1000.0 * (thread_stats.size() + 1);
    if (total_possible_time_ms > 0)
    {
        stats.idle_time_percentage = (total_idle_time_ms / total_possible_time_ms) * 100.0;
    }

    // Calculate load imbalance (coefficient of variation)
    if (!work_per_thread.empty())
    {
        const double mean = static_cast<double>(stats.total_work_executed) / work_per_thread.size();
        double variance = 0.0;

        for (const size_t count : work_per_thread)
        {
            const double diff = static_cast<double>(count) - mean;
            variance += diff * diff;
        }
        variance /= work_per_thread.size();

        const double std_dev = std::sqrt(variance);
        stats.load_imbalance = (mean > 0) ? (std_dev / mean) : 0.0;
    }

    global_stats = stats;

#endif
    return stats;
}

void JobStats::reset()
{
    std::lock_guard lock(stats_mutex);

    for (auto& stats : thread_stats)
        stats = ThreadStats{};

    main_stats = ThreadStats{};

    start_time = std::chrono::steady_clock::now();
    global_stats.last_reset = start_time;
}

void JobStats::log() const
{
#if ENABLE_JOB_STATS
    LOGGER_DEBUG("==== Job System Statistics ====");
    LOGGER_DEBUG("Elapsed Time: {:.2f} seconds", global_stats.elapsed_seconds);

    LOGGER_DEBUG("Work:");
    LOGGER_DEBUG("\tSubmitted: {}", global_stats.total_work_executed);
    LOGGER_DEBUG("\tExecuted: {}", global_stats.total_work_executed);
    LOGGER_DEBUG("\tRate: {:.2f} work/sec", global_stats.total_work_executed / global_stats.elapsed_seconds);
    LOGGER_DEBUG("\tBy Priority:");
    LOGGER_DEBUG("\t\tHigh {}", global_stats.work_by_priority[2]);
    LOGGER_DEBUG("\t\tNormal {}", global_stats.work_by_priority[1]);
    LOGGER_DEBUG("\t\tLow {}", global_stats.work_by_priority[0]);

    LOGGER_DEBUG("Work Execution Time:");
    LOGGER_DEBUG("\tAverage: {:.2f} μs", global_stats.average_work_time_us);
    LOGGER_DEBUG("\tMin: {:.2f} μs", global_stats.min_work_time_ns / 1000.f);
    LOGGER_DEBUG("\tMax: {:.2f} μs", global_stats.max_work_time_ns / 1000.f);

    LOGGER_DEBUG("Work Stealing:");
    LOGGER_DEBUG("\tAttempts: {}", global_stats.total_steal_attempts);
    LOGGER_DEBUG("\tSuccesses: {}", global_stats.total_steal_successes);
    LOGGER_DEBUG("\tSuccess Rate: {:.2f}%", global_stats.steal_success_rate);

    LOGGER_DEBUG("Load Balancing:");
    LOGGER_DEBUG("\tImbalance Coefficient: {:.2f}", global_stats.load_imbalance);
    LOGGER_DEBUG("\t(0.0 = perfect, <0.2 = good, >0.5 = poor)");

    LOGGER_DEBUG("Idle Time:");
    LOGGER_DEBUG("\tTotal: {} ms", global_stats.total_idle_time_ns / 1'000'000.0);
    LOGGER_DEBUG("\tPercentage: {:.2f}", global_stats.idle_time_percentage);
    LOGGER_DEBUG("\tIdle Spins: {}", global_stats.total_idle_spins);

    LOGGER_DEBUG("Per Thread:");
    for (size_t i = 0; i < thread_stats.size(); ++i)
    {
        auto& stats = thread_stats[i];
        LOGGER_DEBUG("\tThread: {}", i);
        LOGGER_DEBUG("\t\tWork Executed: {}", stats.work_executed);
        LOGGER_DEBUG("\t\tWork Stolen: {}", stats.work_stolen);
        LOGGER_DEBUG("\t\tWork Lost: {}", stats.work_lost_to_thieves);
        std::string steal_success;
        if (stats.steal_attempts > 0)
        {
            steal_success = fmt::format("{:.2f}%", static_cast<double>(stats.steal_successes) / stats.steal_attempts * 100.0);
        }
        else
        {
            steal_success = "N/A";
        }
        LOGGER_DEBUG("\t\tSteal Success: {}", steal_success);
    }

    LOGGER_DEBUG("\tMain");
    LOGGER_DEBUG("\t\tWork Executed: {}", main_stats.work_executed);
#else
    LOG_ERROR("Attempted to print job statics but `ENABLE_JOB_STATS` is false");
#endif
}

const std::vector<JobStats::ThreadStats>& JobStats::get_thread_stats() const
{
    return thread_stats;
}

const JobStats::GlobalStats& JobStats::get_global_stats() const
{
    return global_stats;
}
} // portal
