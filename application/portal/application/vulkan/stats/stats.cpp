//
// Created by Jonatan Nevo on 02/03/2025.
//

#include "stats.h"

#include <ranges>

#include "portal/application/vulkan/stats/providers/frame_time_stats_provider.h"
#include "portal/application/vulkan/stats/providers/vulkan_stats_provider.h"
#include "portal/core/assert.h"

using namespace std::chrono_literals;

namespace portal::vulkan
{
const char* to_string(const stats::StatIndex index)
{
    switch (index)
    {
    case stats::StatIndex::FrameTimes:
        return "Frame Times (ms)";
    case stats::StatIndex::CpuCycles:
        return "CPU Cycles (M/s)";
    case stats::StatIndex::CpuInstructions:
        return "CPU Instructions (M/s)";
    case stats::StatIndex::CpuCacheMissRatio:
        return "Cache Miss Ratio (%)";
    case stats::StatIndex::CpuBranchMissRatio:
        return "Branch Miss Ratio (%)";
    case stats::StatIndex::CpuL1Accesses:
        return "CPU L1 Accesses (M/s)";
    case stats::StatIndex::CpuInstrRetired:
        return "CPU Instructions Retired (M/s)";
    case stats::StatIndex::CpuL2Accesses:
        return "CPU L2 Accesses (M/s)";
    case stats::StatIndex::CpuL3Accesses:
        return "CPU L3 Accesses (M/s)";
    case stats::StatIndex::CpuBusReads:
        return "CPU Bus Read Beats (M/s)";
    case stats::StatIndex::CpuBusWrites:
        return "CPU Bus Write Beats (M/s)";
    case stats::StatIndex::CpuMemReads:
        return "CPU Memory Read Instructions (M/s)";
    case stats::StatIndex::CpuMemWrites:
        return "CPU Memory Write Instructions (M/s)";
    case stats::StatIndex::CpuAseSpec:
        return "CPU Speculatively Exec. SIMD Instructions (M/s)";
    case stats::StatIndex::CpuVfpSpec:
        return "CPU Speculatively Exec. FP Instructions (M/s)";
    case stats::StatIndex::CpuCryptoSpec:
        return "CPU Speculatively Exec. Crypto Instructions (M/s)";
    case stats::StatIndex::GpuCycles:
        return "GPU Cycles (M/s)";
    case stats::StatIndex::GpuVertexCycles:
        return "Vertex Cycles (M/s)";
    case stats::StatIndex::GpuLoadStoreCycles:
        return "Load Store Cycles (k/s)";
    case stats::StatIndex::GpuTiles:
        return "Tiles (k/s)";
    case stats::StatIndex::GpuKilledTiles:
        return "Tiles killed by CRC match (k/s)";
    case stats::StatIndex::GpuFragmentJobs:
        return "Fragment Jobs (s)";
    case stats::StatIndex::GpuFragmentCycles:
        return "Fragment Cycles (M/s)";
    case stats::StatIndex::GpuExtReads:
        return "External Reads (M/s)";
    case stats::StatIndex::GpuExtWrites:
        return "External Writes (M/s)";
    case stats::StatIndex::GpuExtReadStalls:
        return "External Read Stalls (M/s)";
    case stats::StatIndex::GpuExtWriteStalls:
        return "External Write Stalls (M/s)";
    case stats::StatIndex::GpuExtReadBytes:
        return "External Read Bytes (MiB/s)";
    case stats::StatIndex::GpuExtWriteBytes:
        return "External Write Bytes (MiB/s)";
    case stats::StatIndex::GpuTexCycles:
        return "Shader Texture Cycles (k/s)";
    }
    return "";
}

static void add_smoothed_value(std::vector<float>& values, const float value, const float alpha)
{
    assert(values.size() >= 2 && "Buffers size should be greater than 2");

    if (values.size() == values.capacity())
    {
        // Shift values to the left to make space at the end and update counters
        std::ranges::rotate(values, values.begin() + 1);
    }

    // Use an exponential moving average to smooth values
    values.back() = value * alpha + *(values.end() - 2) * (1.0f - alpha);
}


Stats::Stats(rendering::RenderContext& render_context, const size_t buffer_size): render_context(render_context),
                                                                                  frame_time_provider(nullptr),
                                                                                  buffer_size(buffer_size)
{
    PORTAL_CORE_ASSERT(buffer_size >= 2, "Buffers size should be greater than 2");
}

Stats::~Stats()
{
    if (stop_worker)
        stop_worker->set_value();

    if (worker_thread.joinable())
        worker_thread.join();
}

void Stats::request_stats(const std::set<stats::StatIndex>& requested_stats, stats::CounterSamplingConfig sampling_config)
{
    if (!providers.empty())
        throw std::runtime_error("Stats must only be requested once");

    this->requested_stats = requested_stats;
    this->sampling_config = sampling_config;

    // Copy the requested stats, so they can be changed by the providers below
    std::set<stats::StatIndex> stats = requested_stats;

    // Initialize our list of providers (in priority order)
    // All supported stats will be removed from the given 'stats' set by the provider's constructor
    // so subsequent providers only see requests for stats that aren't already supported.
    providers.emplace_back(std::make_unique<stats::FrameTimeStatsProvider>(stats));
    providers.emplace_back(std::make_unique<stats::VulkanStatsProvider>(stats, sampling_config, render_context));


    // In continuous sampling mode we still need to update the frame times as if we are polling
    // Store the frame time provider here so we can easily access it later.
    frame_time_provider = providers[0].get();
    for (const auto& stat : requested_stats)
    {
        counters[stat] = std::vector<float>(buffer_size, 0);
    }

    if (sampling_config.mode == stats::CounterSamplingMode::Continuous)
    {
        // Start a thread for continuous sample capture
        stop_worker = std::make_unique<std::promise<void>>();

        worker_thread = std::thread(
            [this]
            {
                continuous_sampling_worker(stop_worker->get_future());
            }
        );

        // Reduce smoothing for continuous sampling
        alpha_smoothing = 0.6f;
    }

    for (const auto& stat_index : requested_stats)
    {
        if (!is_available(stat_index))
        {
            LOG_CORE_WARN_TAG("Statistics", stats::StatsProvider::default_graph_data(stat_index).name + " : not available");
        }
    }
}

void Stats::resize(const size_t width)
{
    // The circular buffer size will be 1/16th of the width of the screen
    // which means every sixteen pixels represent one graph value
    buffer_size = width >> 4;

    for (auto& counter : counters | std::views::values)
    {
        counter.resize(buffer_size);
        counter.shrink_to_fit();
    }
}

bool Stats::is_available(stats::StatIndex index) const
{
    return std::ranges::any_of(providers, [index](const auto& p) { return p->is_available(index); });
}

const stats::StatGraphData& Stats::get_graph_data(stats::StatIndex index) const
{
    for (auto& p : providers)
    {
        if (p->is_available(index))
        {
            return p->get_graph_data(index);
        }
    }
    return stats::StatsProvider::default_graph_data(index);
}

void Stats::update(float delta_time)
{
    switch (sampling_config.mode)
    {
    case stats::CounterSamplingMode::Polling:
        {
            stats::StatsProvider::Counters sample;

            for (const auto& p : providers)
            {
                auto s = p->sample(delta_time);
                sample.insert(s.begin(), s.end());
            }
            push_sample(sample);
            break;
        }
    case stats::CounterSamplingMode::Continuous:
        {
            // Check that we have no pending samples to be shown
            if (pending_samples.empty())
            {
                std::unique_lock<std::mutex> lock(continuous_sampling_mutex);
                if (!should_add_to_continuous_samples)
                {
                    // If we have no pending samples, we let the worker thread
                    // capture samples for the next frame
                    should_add_to_continuous_samples = true;
                }
                else
                {
                    // The worker thread has captured a frame, so we stop it
                    // and read the samples
                    should_add_to_continuous_samples = false;
                    pending_samples.clear();
                    std::swap(pending_samples, continuous_samples);
                }
            }

            if (pending_samples.empty())
                return;

            // Ensure the number of pending samples is capped at a reasonable value
            if (pending_samples.size() > 100)
            {
                // Prefer later samples to new samples.
                std::move(pending_samples.end() - 100, pending_samples.end(), pending_samples.begin());
                pending_samples.erase(pending_samples.begin() + 100, pending_samples.end());

                // If we get to this point, we're not reading samples fast enough, nudge a little ahead.
                fractional_pending_samples += 1.0f;
            }

            // Compute the number of samples to show this frame
            float floating_sample_count = sampling_config.speed * delta_time * static_cast<float>(buffer_size) + fractional_pending_samples;
            // Keep track of the fractional value to avoid speeding up or slowing down too much due to rounding errors.
            // Generally we push very few samples per frame, so this matters.
            fractional_pending_samples = floating_sample_count - std::floor(floating_sample_count);

            auto sample_count = static_cast<int64_t>(floating_sample_count);

            // Clamp the number of samples
            sample_count = std::max<int64_t>(1, std::min<int64_t>(sample_count, pending_samples.size()));

            // Get the frame time stats (not a continuous stat)
            stats::StatsProvider::Counters frame_time_sample = frame_time_provider->sample(delta_time);

            // Push the samples to circular buffers
            std::for_each(
                pending_samples.begin(),
                pending_samples.begin() + sample_count,
                [this, frame_time_sample](auto& s)
                {
                    // Write the correct frame time into the continuous stats
                    s.insert(frame_time_sample.begin(), frame_time_sample.end());
                    // Then push the sample to the counters list
                    this->push_sample(s);
                }
            );
            pending_samples.erase(pending_samples.begin(), pending_samples.begin() + sample_count);

            break;
        }
    }
}

void Stats::begin_sampling(CommandBuffer& cb) const
{
    // Inform the providers
    for (const auto& p : providers)
    {
        p->begin_sampling(cb);
    }
}

void Stats::end_sampling(CommandBuffer& cb) const
{
    // Inform the providers
    for (const auto& p : providers)
    {
        p->end_sampling(cb);
    }
}

void Stats::continuous_sampling_worker(const std::future<void>& should_terminate)
{
    worker_timer.tick();

    for (auto& p : providers)
    {
        p->continuous_sample(0.0f);
    }

    while (should_terminate.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    {
        auto delta_time = static_cast<float>(worker_timer.tick());
        auto interval = std::chrono::duration_cast<std::chrono::duration<float>>(sampling_config.interval).count();


        // Ensure we wait for the interval specified in config
        if (delta_time < interval)
        {
            std::this_thread::sleep_for(std::chrono::duration<float>(interval - delta_time));
            delta_time += static_cast<float>(worker_timer.tick());
        }

        // Sample counters
        stats::StatsProvider::Counters sample;
        for (const auto& p : providers)
        {
            stats::StatsProvider::Counters s = p->continuous_sample(delta_time);
            sample.insert(s.begin(), s.end());
        }

        // Add the new sample to the vector of continuous samples
        {
            std::unique_lock<std::mutex> lock(continuous_sampling_mutex);
            if (should_add_to_continuous_samples)
            {
                continuous_samples.push_back(sample);
            }
        }
    }
}

void Stats::push_sample(const stats::StatsProvider::Counters& sample)
{
    for (auto& [index, values] : counters)
    {
        // Find the counter matching this StatIndex in the Sample
        const auto& smp = sample.find(index);
        if (smp == sample.end())
            continue;

        const auto measurement = static_cast<float>(smp->second.result);
        add_smoothed_value(values, measurement, alpha_smoothing);
    }
}
}
