//
// Created by Jonatan Nevo on 02/03/2025.
//

#pragma once
#include <cstdint>
#include <ctime>
#include <future>
#include <map>
#include <set>
#include <vector>

#include "portal/application/vulkan/rendering/render_context.h"
#include "portal/application/vulkan/stats/common.h"
#include "portal/application/vulkan/stats/stats_provider.h"
#include "portal/core/timer.h"

namespace portal::vulkan
{
namespace rendering
{
    class RenderContext;
}

class Device;
class CommandBuffer;


/*
 * @brief Helper class for querying statistics about the CPU and the GPU
 */
class Stats
{
public:
    /**
     * @brief Constructs a Stats object
     * @param render_context The RenderContext for this sample
     * @param buffer_size Size of the circular buffers
     */
    explicit Stats(rendering::RenderContext& render_context, size_t buffer_size = 16);

    /**
     * @brief Destroys the Stats object
     */
    ~Stats();

    /**
     * @brief Request specific set of stats to be collected
     * @param requested_stats Set of stats to be collected if available
     * @param sampling_config Sampling mode configuration (polling or continuous)
     */
    void request_stats(
        const std::set<stats::StatIndex>& requested_stats,
        stats::CounterSamplingConfig sampling_config = {stats::CounterSamplingMode::Polling}
    );

    /**
     * @brief Resizes the stats buffers according to the width of the screen
     * @param width The width of the screen
     */
    void resize(size_t width);

    /**
     * @brief Checks if an enabled stat is available in the current platform
     * @param index The stat index
     * @return True if the stat is available, false otherwise
     */
    bool is_available(stats::StatIndex index) const;

    /**
     * @brief Returns data relevant for graphing a specific statistic
     * @param index The stat index of the data requested
     * @return The data of the specified stat
     */
    [[nodiscard]] const stats::StatGraphData& get_graph_data(stats::StatIndex index) const;

    /**
     * @brief Returns the collected data for a specific statistic
     * @param index The stat index of the data requested
     * @return The data of the specified stat
     */
    const std::vector<float>& get_data(const stats::StatIndex index) const
    {
        return counters.at(index);
    };

    /**
     * @return The requested stats
     */
    [[nodiscard]] const std::set<stats::StatIndex>& get_requested_stats() const
    {
        return requested_stats;
    }

    /**
     * @brief Update statistics, must be called after every frame
     * @param delta_time Time since last update
     */
    void update(float delta_time);

    /**
     * @brief A command buffer that we want to collect stats about has just begun
     *
     * Some stats providers (like the Vulkan extension one) can only collect stats
     * about the execution of a specific command buffer. In those cases we need to
     * know when a command buffer has begun and when it's about to end so that we
     * can inject some extra commands into the command buffer to control the stats
     * collection. This method tells the stats provider that a command buffer has
     * begun so that can happen. The command buffer must be in a recording state
     * when this method is called.
     * @param cb The command buffer
     */
    void begin_sampling(CommandBuffer& cb) const;

    /**
     * @brief A command buffer that we want to collect stats about is about to be ended
     *
     * Some stats providers (like the Vulkan extension one) can only collect stats
     * about the execution of a specific command buffer. In those cases we need to
     * know when a command buffer has begun and when it's about to end so that we
     * can inject some extra commands into the command buffer to control the stats
     * collection. This method tells the stats provider that a command buffer is
     * about to be ended so that can happen. The command buffer must be in a recording
     * state when this method is called.
     * @param cb The command buffer
     */
    void end_sampling(CommandBuffer& cb) const;

private:
    /// A value which helps keep a steady pace of continuous samples output.
    float fractional_pending_samples{0.0f};
    /// The worker thread function for continuous sampling;
    /// it adds a new entry to continuous_samples at every interval
    void continuous_sampling_worker(const std::future<void>& should_terminate);
    /// Updates circular buffers for CPU and GPU counters
    void push_sample(const stats::StatsProvider::Counters& sample);

    /// The render context
    rendering::RenderContext& render_context;
    /// Stats that were requested - they may not all be available
    std::set<stats::StatIndex> requested_stats;
    /// Provider that tracks frame times
    stats::StatsProvider* frame_time_provider;
    /// A list of stats providers to use in priority order
    std::vector<std::unique_ptr<stats::StatsProvider>> providers;
    /// Counter sampling configuration
    stats::CounterSamplingConfig sampling_config;
    /// Size of the circular buffers
    size_t buffer_size;
    /// Timer used in the main thread to compute delta time
    Timer main_timer;
    /// Timer used by the worker thread to throttle counter sampling
    Timer worker_timer;
    /// Alpha smoothing for running average
    float alpha_smoothing{0.2f};
    /// Circular buffers for counter data
    std::map<stats::StatIndex, std::vector<float>> counters{};
    /// Worker thread for continuous sampling
    std::thread worker_thread;
    /// Promise to stop the worker thread
    std::unique_ptr<std::promise<void>> stop_worker;
    /// A mutex for accessing measurements during continuous sampling
    std::mutex continuous_sampling_mutex;
    /// The samples read during continuous sampling
    std::vector<stats::StatsProvider::Counters> continuous_samples;
    /// A flag specifying if the worker thread should add entries to continuous_samples
    bool should_add_to_continuous_samples{false};
    /// The samples waiting to be displayed
    std::vector<stats::StatsProvider::Counters> pending_samples;
};
}


