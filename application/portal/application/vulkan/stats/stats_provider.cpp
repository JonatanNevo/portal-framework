//
// Created by Jonatan Nevo on 15/03/2025.
//

#include "stats_provider.h"

namespace portal::vulkan::stats
{

// Default graphing values for stats. May be overridden by individual providers.
std::map<StatIndex, StatGraphData> StatsProvider::default_graph_map{
    // clang-format off
    // StatIndex                             Name shown in graph                                             Format                     Scale                                    Fixed_max Max_value
    {StatIndex::FrameTimes,           {"Frame Times",                                 "{:3.1f} ms",    1000.0f}},
    {StatIndex::CpuCycles,            {"CPU Cycles",                                  "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::CpuInstructions,      {"CPU Instructions",                            "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::CpuCacheMissRatio,    {"Cache Miss Ratio",                            "{:3.1f}%",      100.0f,                       true,     100.0f}},
    {StatIndex::CpuBranchMissRatio,   {"Branch Miss Ratio",                           "{:3.1f}%",      100.0f,                       true,     100.0f}},
    {StatIndex::CpuL1Accesses,        {"CPU L1 Accesses",                             "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::CpuInstrRetired,      {"CPU Instructions Retired",                    "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::CpuL2Accesses,        {"CPU L2 Accesses",                             "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::CpuL3Accesses,        {"CPU L3 Accesses",                             "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::CpuBusReads,          {"CPU Bus Read Beats",                          "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::CpuBusWrites,         {"CPU Bus Write Beats",                         "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::CpuMemReads,          {"CPU Memory Read Instructions",                "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::CpuMemWrites,         {"CPU Memory Write Instructions",               "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::CpuAseSpec,           {"CPU Speculatively Exec. SIMD Instructions",   "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::CpuVfpSpec,           {"CPU Speculatively Exec. FP Instructions",     "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::CpuCryptoSpec,        {"CPU Speculatively Exec. Crypto Instructions", "{:4.1f} M/s",   static_cast<float>(1e-6)}},

    {StatIndex::GpuCycles,            {"GPU Cycles",                                  "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::GpuVertexCycles,      {"Vertex Cycles",                               "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::GpuLoadStoreCycles,   {"Load Store Cycles",                           "{:4.0f} k/s",   static_cast<float>(1e-6)}},
    {StatIndex::GpuTiles,             {"Tiles",                                       "{:4.1f} k/s",   static_cast<float>(1e-3)}},
    {StatIndex::GpuKilledTiles,       {"Tiles killed by CRC match",                   "{:4.1f} k/s",   static_cast<float>(1e-3)}},
    {StatIndex::GpuFragmentJobs,      {"Fragment Jobs",                               "{:4.0f}/s"}},
    {StatIndex::GpuFragmentCycles,    {"Fragment Cycles",                             "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::GpuTexCycles,         {"Shader Texture Cycles",                       "{:4.0f} k/s",   static_cast<float>(1e-3)}},
    {StatIndex::GpuExtReads,          {"External Reads",                              "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::GpuExtWrites,         {"External Writes",                             "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::GpuExtReadStalls,     {"External Read Stalls",                        "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::GpuExtWriteStalls,    {"External Write Stalls",                       "{:4.1f} M/s",   static_cast<float>(1e-6)}},
    {StatIndex::GpuExtReadBytes,      {"External Read Bytes",                         "{:4.1f} MiB/s", 1.0f / (1024.0f * 1024.0f)}},
    {StatIndex::GpuExtWriteBytes,     {"External Write Bytes",                        "{:4.1f} MiB/s", 1.0f / (1024.0f * 1024.0f)}},
    // clang-format on
};

const StatGraphData& StatsProvider::default_graph_data(const StatIndex index)
{
    return default_graph_map.at(index);
}
} // portal
