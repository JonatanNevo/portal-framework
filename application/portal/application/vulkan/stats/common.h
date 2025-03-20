//
// Created by Jonatan Nevo on 15/03/2025.
//

#pragma once

#include <chrono>
#include <string>
#include <utility>

namespace portal::vulkan::stats
{
/**
 * @brief Handles of stats to be optionally enabled in @ref Stats
 */
enum class StatIndex
{
    FrameTimes,
    CpuCycles,
    CpuInstructions,
    CpuCacheMissRatio,
    CpuBranchMissRatio,
    CpuL1Accesses,
    CpuInstrRetired,
    CpuL2Accesses,
    CpuL3Accesses,
    CpuBusReads,
    CpuBusWrites,
    CpuMemReads,
    CpuMemWrites,
    CpuAseSpec,
    CpuVfpSpec,
    CpuCryptoSpec,

    GpuCycles,
    GpuVertexCycles,
    GpuLoadStoreCycles,
    GpuTiles,
    GpuKilledTiles,
    GpuFragmentJobs,
    GpuFragmentCycles,
    GpuExtReads,
    GpuExtWrites,
    GpuExtReadStalls,
    GpuExtWriteStalls,
    GpuExtReadBytes,
    GpuExtWriteBytes,
    GpuTexCycles,
};

struct StatIndexHash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};

enum class StatScaling
{
    // The stat is not scaled
    None,
    // The stat is scaled by delta time, useful for per-second values
    ByDeltaTime,
    // The stat is scaled by another counter, useful for ratios
    ByCounter
};

enum class CounterSamplingMode
{
    /// Sample counters only when calling update()
    Polling,
    /// Sample counters continuously, update circular buffers when calling update()
    Continuous
};

struct CounterSamplingConfig
{
    /// Sampling mode (polling or continuous)
    CounterSamplingMode mode{};
    /// Sampling interval in continuous mode
    std::chrono::milliseconds interval{1};
    /// Speed of circular buffer updates in continuous mode;
    /// at speed = 1.0f a new sample is displayed over 1 second.
    float speed = 0.5f;
};

// Per-statistic graph data
class StatGraphData
{
public:
    /**
     * @brief Constructs data for the graph
     * @param name Name of the Stat
     * @param format Format of the label
     * @param scale_factor Any scaling to apply to the data
     * @param has_fixed_max Whether the data should have a fixed max value
     * @param max_value The maximum value to use
     */
    StatGraphData(
        std::string name,
        std::string format,
        const float scale_factor = 1.0f,
        const bool has_fixed_max = false,
        const float max_value = 0.0f
    ): name(std::move(name)),
       format{std::move(format)},
       scale_factor{scale_factor},
       has_fixed_max{has_fixed_max},
       max_value{max_value} {}

    StatGraphData() = default;

    std::string name{};
    std::string format{};
    float scale_factor{};
    bool has_fixed_max{};
    float max_value{};
};
}
