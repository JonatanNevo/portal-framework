//
// Created by Jonatan Nevo on 15/03/2025.
//

#include "frame_time_stats_provider.h"

namespace portal::vulkan::stats
{
FrameTimeStatsProvider::FrameTimeStatsProvider(std::set<StatIndex>& requested_stats)
{
    // We always, and only, support StatIndex::frame_times since it's handled directly by us.
    // Remove from requested set to stop other providers looking for it.
    requested_stats.erase(StatIndex::FrameTimes);
}

bool FrameTimeStatsProvider::is_available(StatIndex index) const
{
    // We only support StatIndex::frame_times
    return index == StatIndex::FrameTimes;
}

StatsProvider::Counters FrameTimeStatsProvider::sample(const float delta_time)
{
    Counters res;
    // frame_times comes directly from delta_time
    res[StatIndex::FrameTimes].result = delta_time;
    return res;
}
}
