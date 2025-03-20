//
// Created by Jonatan Nevo on 09/03/2025.
//

#include "query_pool.h"

#include "portal/application/vulkan/device.h"

namespace portal::vulkan
{
QueryPool::QueryPool(Device& d, const vk::QueryPoolCreateInfo& info): VulkanResource(nullptr, &d)
{
    set_handle(get_device_handle().createQueryPool(info));
}

QueryPool::QueryPool(QueryPool&& pool) noexcept: VulkanResource(std::move(pool)) {}

QueryPool::~QueryPool()
{
    if (has_handle())
        get_device_handle().destroyQueryPool(get_handle());
}

void QueryPool::host_reset(const uint32_t first_query, const uint32_t query_count)
{
    PORTAL_CORE_ASSERT(
        get_device().is_enabled("VK_EXT_host_query_reset"),
        "VK_EXT_host_query_reset needs to be enabled to call QueryPool::host_reset"
    );

    get_device_handle().resetQueryPoolEXT(get_handle(), first_query, query_count);
}

vk::Result QueryPool::get_results(
    const uint32_t first_query,
    const uint32_t num_queries,
    const size_t result_bytes,
    void* results,
    const vk::DeviceSize stride,
    const vk::QueryResultFlags flags
)
{
    return get_device_handle().getQueryPoolResults(get_handle(), first_query, num_queries, result_bytes, results, stride, flags);
}
} // portal
