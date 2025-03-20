//
// Created by Jonatan Nevo on 15/03/2025.
//

#include "vulkan_stats_provider.h"

#include <regex>

#include "portal/application/vulkan/device.h"
#include "portal/application/vulkan/physical_device.h"
#include "portal/application/vulkan/query_pool.h"

namespace portal::vulkan::stats
{
static double get_counter_value(
    const vk::PerformanceCounterResultKHR& result,
    vk::PerformanceCounterStorageKHR storage
)
{
    switch (storage)
    {
    case vk::PerformanceCounterStorageKHR::eInt32:
        return static_cast<double>(result.int32);
    case vk::PerformanceCounterStorageKHR::eInt64:
        return static_cast<double>(result.int64);
    case vk::PerformanceCounterStorageKHR::eUint32:
        return static_cast<double>(result.uint32);
    case vk::PerformanceCounterStorageKHR::eUint64:
        return static_cast<double>(result.uint64);
    case vk::PerformanceCounterStorageKHR::eFloat32:
        return static_cast<double>(result.float32);
    case vk::PerformanceCounterStorageKHR::eFloat64:
        return result.float64;
    default:
        return 0.0;
    }
}

VulkanStatsProvider::VulkanStatsProvider(
    std::set<StatIndex>& requested_stats,
    const CounterSamplingConfig& sampling_config,
    rendering::RenderContext& render_context
): render_context(render_context)
{
    // Check all the Vulkan capabilities we require are present
    if (!is_supported(sampling_config))
        return;

    Device& device = render_context.get_device();
    const PhysicalDevice& gpu = device.get_gpu();

    has_timestamps = gpu.get_properties().limits.timestampComputeAndGraphics;
    timestamp_period = gpu.get_properties().limits.timestampPeriod;

    // Interrogate device for supported stats
    const uint32_t queue_family_index = device.get_queue_family_index(vk::QueueFlagBits::eGraphics);
    // Query number of available counters
    auto [counters, descriptions] = gpu.get_handle().enumerateQueueFamilyPerformanceQueryCountersKHR(queue_family_index);

    // Every vendor has a different set of performance counters each
    // with different names. Match them to the stats we want, where available.
    if (!fill_vendor_data())
        return;

    bool performance_impact = false;
    // Now build stat_data by matching vendor_data to Vulkan counter data
    for (auto& [index, init] : vendor_data)
    {
        if (!requested_stats.contains(index))
            continue; // We weren't asked for this stat

        bool found_ctr = false;
        bool found_div = (init.divisor_name.empty());
        uint32_t ctr_idx = 0, div_idx = 0;

        std::regex name_regex(init.name);
        std::regex div_regex(init.divisor_name);

        for (uint32_t i = 0; !(found_ctr && found_div) && i < descriptions.size(); i++)
        {
            if (!found_ctr && std::regex_match(descriptions[i].name.data(), name_regex))
            {
                ctr_idx = i;
                found_ctr = true;
            }
            if (!found_div && std::regex_match(descriptions[i].name.data(), div_regex))
            {
                div_idx = i;
                found_div = true;
            }
        }

        if (found_ctr && found_div)
        {
            if (descriptions[ctr_idx].flags & vk::PerformanceCounterDescriptionFlagBitsKHR::ePerformanceImpacting ||
                (!init.divisor_name.empty() && descriptions[div_idx].flags & vk::PerformanceCounterDescriptionFlagBitsKHR::ePerformanceImpacting))
                performance_impact = true;

            // Record the counter data
            counter_indices.emplace_back(ctr_idx);
            if (init.divisor_name.empty())
                stat_data[index] = StatData(ctr_idx, counters[ctr_idx].storage);
            else
            {
                counter_indices.emplace_back(div_idx);
                stat_data[index] = StatData(
                    ctr_idx,
                    counters[ctr_idx].storage,
                    init.scaling,
                    div_idx,
                    counters[div_idx].storage
                );
            }
        }
    }

    if (performance_impact)
        LOG_CORE_WARN_TAG("Statistics", "The collection of performance counters may impact performance");

    if (counter_indices.empty())
        return; // No stats available

    // Acquire the profiling lock, without which we can't collect stats
    vk::AcquireProfilingLockInfoKHR info{};
    info.timeout = 2000000000; // 2 seconds (in ns)
    device.get_handle().acquireProfilingLockKHR(info);

    // Now we know the counters and that we can collect them, make a query pool for the results.
    if (!create_query_pools(queue_family_index))
    {
        stat_data.clear();
        counter_indices.clear();
        return;
    }

    // These stats are fully supported by this provider and in a single pass, so remove
    // from the requested set.
    // Subsequent providers will then only look for things that aren't already supported.
    for (const auto& s : stat_data)
    {
        requested_stats.erase(s.first);
    }
}

VulkanStatsProvider::~VulkanStatsProvider()
{
    if (!stat_data.empty())
    {
        // Release profiling lock
        render_context.get_device().get_handle().releaseProfilingLockKHR();
    }
}

bool VulkanStatsProvider::is_available(const StatIndex index) const
{
    return stat_data.contains(index);
}

const StatGraphData& VulkanStatsProvider::get_graph_data(const StatIndex index) const
{
    PORTAL_CORE_ASSERT(is_available(index), "VulkanStatsProvider::get_graph_data() called with invalid StatIndex");

    const auto& data = vendor_data.find(index)->second;
    if (data.has_vendor_graph_data)
    {
        return data.graph_data;
    }

    return default_graph_map[index];
}

StatsProvider::Counters VulkanStatsProvider::sample(float delta_time)
{
    Counters out;
    if (!query_pool || queries_ready == 0)
    {
        return out;
    }

    const uint32_t active_frame_idx = render_context.get_active_frame_index();
    const auto stride = sizeof(VkPerformanceCounterResultKHR) * counter_indices.size();
    std::vector<vk::PerformanceCounterResultKHR> results(counter_indices.size());

    const auto result = query_pool->get_results(
        active_frame_idx,
        1,
        results.size() * sizeof(vk::PerformanceCounterResultKHR),
        results.data(),
        stride,
        vk::QueryResultFlagBits::eWait
    );

    if (result != vk::Result::eSuccess)
    {
        return out;
    }

    // Use timestamps to get a more accurate delta if available
    delta_time = get_best_delta_time(delta_time);

    // Parse the results - they are in the order we gave in counter_indices
    for (const auto& [index, data] : stat_data)
    {
        const bool need_divisor = (stat_data[index].scaling == StatScaling::ByCounter);
        double divisor_value = 1.0;
        double value = 0.0;
        bool found_ctr = false, found_div = !need_divisor;

        for (uint32_t i = 0; !(found_ctr && found_div) && i < counter_indices.size(); i++)
        {
            if (data.counter_index == counter_indices[i])
            {
                value = get_counter_value(results[i], stat_data[index].storage);
                found_ctr = true;
            }
            if (need_divisor && data.divisor_counter_index == counter_indices[i])
            {
                divisor_value = get_counter_value(results[i], stat_data[index].divisor_storage);
                found_div = true;
            }
        }

        if (found_ctr && found_div)
        {
            if (stat_data[index].scaling == StatScaling::ByDeltaTime && delta_time != 0.0)
            {
                value /= delta_time;
            }
            else if (stat_data[index].scaling == StatScaling::ByCounter && divisor_value != 0.0)
            {
                value /= divisor_value;
            }
            out[index].result = value;
        }
    }

    // Now reset the query we just fetched the results from
    query_pool->host_reset(active_frame_idx, 1);

    --queries_ready;

    return out;
}

void VulkanStatsProvider::begin_sampling(CommandBuffer& cb)
{
    const uint32_t active_frame_idx = render_context.get_active_frame_index();
    if (timestamp_pool)
    {
        // We use TimestampQueries when available to provide a more accurate delta_time.
        // These counters are from a single command buffer execution, but the passed
        // delta time is a frame-to-frame s/w measure. A timestamp query in the the cmd
        // buffer gives the actual elapsed time when the counters were measured.
        cb.reset_query_pool(*timestamp_pool, active_frame_idx * 2, 1);
        cb.write_timestamp(
            vk::PipelineStageFlagBits::eBottomOfPipe,
            *timestamp_pool,
            active_frame_idx * 2
        );
    }

    if (query_pool)
    {
        cb.begin_query(*query_pool, active_frame_idx, vk::QueryControlFlags{});
    }
}

void VulkanStatsProvider::end_sampling(CommandBuffer& cb)
{
    uint32_t active_frame_idx = render_context.get_active_frame_index();
    if (query_pool)
    {
        // Perform a barrier to ensure all previous commands complete before ending the query
        // This does not block later commands from executing as we use BOTTOM_OF_PIPE in the
        // dst stage mask
        vkCmdPipelineBarrier(
            cb.get_handle(),
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            0,
            nullptr
        );
        cb.end_query(*query_pool, active_frame_idx);

        ++queries_ready;
    }

    if (timestamp_pool)
    {
        cb.reset_query_pool(*timestamp_pool, active_frame_idx * 2 + 1, 1);
        cb.write_timestamp(
            vk::PipelineStageFlagBits::eBottomOfPipe,
            *timestamp_pool,
            active_frame_idx * 2 + 1
        );
    }
}

bool VulkanStatsProvider::is_supported(const CounterSamplingConfig& sampling_config) const
{
    // Continuous sampling mode cannot be supported by VK_KHR_performance_query
    if (sampling_config.mode == CounterSamplingMode::Continuous)
    {
        return false;
    }

    Device& device = render_context.get_device();
    // The VK_KHR_performance_query must be available and enabled
    if (!(device.is_enabled("VK_KHR_performance_query") && device.is_enabled("VK_EXT_host_query_reset")))
    {
        return false;
    }

    // Check the performance query features flag.
    // Note: VK_KHR_get_physical_device_properties2 is a pre-requisite of VK_KHR_performance_query
    // so must be present.
    VkPhysicalDevicePerformanceQueryFeaturesKHR perf_query_features{};
    perf_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR;

    VkPhysicalDeviceFeatures2KHR device_features{};
    device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
    device_features.pNext = &perf_query_features;

    vkGetPhysicalDeviceFeatures2KHR(device.get_gpu().get_handle(), &device_features);
    return perf_query_features.performanceCounterQueryPools;
}

bool VulkanStatsProvider::fill_vendor_data()
{
    const auto& pd_props = render_context.get_device().get_gpu().get_properties();
    if (pd_props.vendorID == 0x14E4) // Broadcom devices
    {
        LOG_CORE_INFO_TAG("Statistics", "Using Vulkan performance counters from Broadcom device");

        // NOTE: The names here are actually regular-expressions.
        // Counter names can change between hardware variants for the same vendor,
        // so regular expression names mean that multiple h/w variants can be easily supported.
        vendor_data = {
            {StatIndex::GpuCycles, {"cycle_count"}},
            {StatIndex::GpuVertexCycles, {"gpu_vertex_cycles"}},
            {StatIndex::GpuFragmentCycles, {"gpu_fragment_cycles"}},
            {StatIndex::GpuFragmentJobs, {"render_jobs_completed"}},
            {StatIndex::GpuExtReads, {"gpu_mem_reads"}},
            {StatIndex::GpuExtWrites, {"gpu_mem_writes"}},
            {StatIndex::GpuExtReadBytes, {"gpu_bytes_read"}},
            {StatIndex::GpuExtWriteBytes, {"gpu_bytes_written"}},
        };

        // Override vendor-specific graph data
        vendor_data.at(StatIndex::GpuVertexCycles).set_vendor_graph_data({"Vertex/Coord/User Cycles", "{:4.1f} M/s", static_cast<float>(1e-6)});
        vendor_data.at(StatIndex::GpuFragmentJobs).set_vendor_graph_data({"Render Jobs", "{:4.0f}/s"});


        return true;
    }
    {
        // Unsupported vendor
        return true;
    }
}

bool VulkanStatsProvider::create_query_pools(uint32_t queue_family_index)
{
    Device& device = render_context.get_device();
    const PhysicalDevice& gpu = device.get_gpu();
    auto num_framebuffers = static_cast<uint32_t>(render_context.get_render_frames().size());

    // Now we know the available counters, we can build a query pool that will collect them.
    // We will check that the counters can be collected in a single pass. Multi-pass would
    // be a big performance hit so for these samples, we don't want to use it.
    vk::QueryPoolPerformanceCreateInfoKHR perf_create_info{};
    perf_create_info.queueFamilyIndex = queue_family_index;
    perf_create_info.counterIndexCount = static_cast<uint32_t>(counter_indices.size());
    perf_create_info.pCounterIndices = counter_indices.data();

    const uint32_t passes_needed = gpu.get_queue_family_performance_query_passes(&perf_create_info);
    if (passes_needed != 1)
    {
        // Needs more than one pass, remove all our supported stats
        LOG_CORE_WARN_TAG("Vulkan", "Requested Vulkan stats require multiple passes, we won't collect them");
        return false;
    }

    // We will need a query pool to report the stats back to us
    vk::QueryPoolCreateInfo pool_create_info{};
    pool_create_info.pNext = &perf_create_info;
    pool_create_info.queryType = vk::QueryType::ePerformanceQueryKHR;
    pool_create_info.queryCount = num_framebuffers;

    query_pool = std::make_unique<QueryPool>(device, pool_create_info);

    if (!query_pool)
    {
        LOG_CORE_WARN_TAG("Vulkan", "Failed to create performance query pool");
        return false;
    }

    // Reset the query pool before first use. We cannot do these in the command buffer
    // as that is invalid usage for performance queries due to the potential for multiple
    // passes being required.
    query_pool->host_reset(0, num_framebuffers);

    if (has_timestamps)
    {
        // If we support timestamp queries we will use those to more accurately measure
        // the time spent executing a command buffer than just a frame-to-frame timer
        // in software.
        vk::QueryPoolCreateInfo timestamp_pool_create_info{};
        timestamp_pool_create_info.queryType = vk::QueryType::eTimestamp;
        timestamp_pool_create_info.queryCount = num_framebuffers * 2; // 2 timestamps per frame (start & end)

        timestamp_pool = std::make_unique<QueryPool>(device, timestamp_pool_create_info);
    }

    return true;
}

float VulkanStatsProvider::get_best_delta_time(float sw_delta_time) const
{
    if (!timestamp_pool)
    {
        return sw_delta_time;
    }

    float delta_time = sw_delta_time;
    // Query the timestamps to get an accurate delta time
    std::array<uint64_t, 2> timestamps{};
    const uint32_t active_frame_idx = render_context.get_active_frame_index();

    const auto result = timestamp_pool->get_results(
        active_frame_idx * 2,
        2,
        timestamps.size() * sizeof(uint64_t),
        timestamps.data(),
        sizeof(uint64_t),
        vk::QueryResultFlagBits::eWait | vk::QueryResultFlagBits::e64
    );

    if (result == vk::Result::eSuccess)
    {
        const float elapsed_ns = timestamp_period * static_cast<float>(timestamps[1] - timestamps[0]);
        delta_time = elapsed_ns * 0.000000001f;
    }

    return delta_time;
}
}
