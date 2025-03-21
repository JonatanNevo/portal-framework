//
// Created by Jonatan Nevo on 21/03/2025.
//

#include "asset_registry.h"
#include <portal/core/log.h>

#include "portal/core/thread.h"

#define LOG_ASSERT_REGISTRY 0
#if LOG_ASSERT_REGISTRY
#define ASSET_LOG(...) LOG_CORE_TRACE_TAG("Asset", __VA_ARGS__)
#else
#define ASSET_LOG(...)
#endif

namespace portal {
const AssetMetadata& AssetRegistry::get(const AssetHandle& handle)
{
    std::scoped_lock lock(registry_mutex);
    PORTAL_CORE_ASSERT(registry.contains(handle), "Asset handle not found in registry");
    ASSET_LOG("Retriving const handle {}", handle);
    return registry.at(handle);
}

void AssetRegistry::set(const AssetHandle& handle, const AssetMetadata& metadata)
{
    std::scoped_lock lock(registry_mutex);
    PORTAL_CORE_ASSERT(metadata.handle == handle, "Metadata handle does not match the handle");
    PORTAL_CORE_ASSERT(handle != 0, "Invalid handle");
    PORTAL_CORE_ASSERT(is_main_thread(), "AssetRegistry::set() has been called from other than the main thread!");
    registry[handle] = metadata;
}

bool AssetRegistry::contains(const AssetHandle& handle)
{
    std::scoped_lock lock(registry_mutex);
    ASSET_LOG("Contains handle {}", handle);
    return registry.contains(handle);
}

size_t AssetRegistry::remove(const AssetHandle& handle)
{
    std::scoped_lock lock(registry_mutex);
    ASSET_LOG("Removing handle {}", handle);
    return registry.erase(handle);
}

void AssetRegistry::clear()
{
    std::scoped_lock lock(registry_mutex);
    ASSET_LOG("Clearing registry");
    registry.clear();
}
} // portal