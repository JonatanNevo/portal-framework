//
// Created by Jonatan Nevo on 09/03/2025.
//

#pragma once
#include "portal/application/vulkan/base/vulkan_resource.h"

namespace portal::vulkan
{
class Device;

/**
 * @brief Represents a Vulkan Query Pool
 */
class QueryPool final : public VulkanResource<vk::QueryPool>
{
public:
    /**
     * @brief Creates a Vulkan Query Pool
     * @param d The device to use
     * @param info Creation details
     */
    QueryPool(Device& d, const vk::QueryPoolCreateInfo& info);
    QueryPool(QueryPool&& pool) noexcept;
    ~QueryPool() override;

    QueryPool(const QueryPool&) = delete;
    QueryPool& operator=(const QueryPool&) = delete;
    QueryPool& operator=(QueryPool&&) = delete;

    /**
     * @brief Reset a range of queries in the query pool. Only call if VK_EXT_host_query_reset is enabled.
     * @param first_query The first query to reset
     * @param query_count The number of queries to reset
     */
    void host_reset(uint32_t first_query, uint32_t query_count);

    /**
     * @brief Get query pool results
     * @param first_query The initial query index
     * @param num_queries The number of queries to read
     * @param result_bytes The number of bytes in the results array
     * @param results Array of bytes result_bytes long
     * @param stride The stride in bytes between results for individual queries
     * @param flags A bitmask of VkQueryResultFlagBits
     */
    vk::Result get_results(
        uint32_t first_query,
        uint32_t num_queries,
        size_t result_bytes,
        void* results,
        vk::DeviceSize stride,
        vk::QueryResultFlags flags
    );
};
} // portal
