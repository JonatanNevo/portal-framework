//
// Created by Jonatan Nevo on 15/03/2025.
//

#pragma once

#include <map>
#include <set>
#include <unordered_map>

#include "portal/application/vulkan/stats/common.h"


namespace portal::vulkan
{
class CommandBuffer;
}

namespace portal::vulkan::stats
{
/**
 * @brief Abstract interface for all StatsProvider classes
 */
class StatsProvider
{
public:
    struct Counter
    {
        double result;
    };

    using Counters = std::unordered_map<stats::StatIndex, Counter, StatIndexHash>;

    /**
     * @brief Virtual Destructor
     */
    virtual ~StatsProvider() = default;

    /**
     * @brief Checks if this provider can supply the given enabled stat
     * @param index The stat index
     * @return True if the stat is available, false otherwise
     */
    [[nodiscard]] virtual bool is_available(StatIndex index) const = 0;

    /**
     * @brief Retrieve graphing data for the given enabled stat
     * @param index The stat index
     */
    [[nodiscard]] virtual const StatGraphData& get_graph_data(const StatIndex index) const
    {
        return default_graph_map.at(index);
    }

    /**
     * @brief Retrieve default graphing data for the given stat
     * @param index The stat index
     */
    static const StatGraphData& default_graph_data(StatIndex index);

    /**
     * @brief Retrieve a new sample set
     * @param delta_time Time since last sample
     */
    virtual Counters sample(float delta_time) = 0;

    /**
     * @brief Retrieve a new sample set from continuous sampling
     * @param delta_time Time since last sample
     */
    virtual Counters continuous_sample(float delta_time)
    {
        return {};
    }

    /**
     * @brief A command buffer that we want stats about has just begun
     * @param cb The command buffer
     */
    virtual void begin_sampling(CommandBuffer& cb) {}

    /**
     * @brief A command buffer that we want stats about, is about to be ended
     * @param cb The command buffer
     */
    virtual void end_sampling(CommandBuffer& cb) {}

protected:
    static std::map<StatIndex, StatGraphData> default_graph_map;
};
} // portal
