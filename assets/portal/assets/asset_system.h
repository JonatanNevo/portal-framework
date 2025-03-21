//
// Created by Jonatan Nevo on 21/03/2025.
//

#pragma once
#include <queue>
#include <portal/core/reference.h>
#include <portal/core/thread.h>

#include "portal/assets/asset.h"
#include "portal/assets/asset_metadata.h"
#include "portal/assets/asset_registry.h"

namespace portal
{
class AssetRegistry;

class AssetSystem final : public CountedReference
{
public:
    AssetSystem();
    ~AssetSystem() override;

    void set_registry(const Reference<AssetRegistry>& registry);

    /**
     * Queues an asset to be loaded
     *
     * @param metadata The asset
     */
    void queue_asset_load(const AssetMetadata& metadata);

    /**
     * Get an asset immediately (on the asset thread)
     * if the asset need to be loaded, it will be loaded asynchronously and transferred back to the main thread at the next asset sync
     *
     * @param request The asset
     */
    Reference<Asset> get_asset(const AssetMetadata& request);

    /**
     * Retrieve assets that has been loaded
     */
    bool retrieve_ready_assets(std::vector<AssetLoadResponse>& out_assets);

    // Replace the currently loaded asset collection with the given loadedAssets.
    // This effectively takes a "thread local" snapshot of the asset manager's loaded assets.
    void update_loaded_asset_list(const std::unordered_map<AssetHandle, Reference<Asset>>& assets);

    void mark_asset_as_loaded(const AssetHandle& handle, const Reference<Asset>& asset);

    void stop();
    void stop_and_wait();

    void asset_monitor_update();

private:
    void asset_thread_func();

    std::filesystem::path get_filesystem_path(const AssetMetadata& metadata);

    void ensure_all_loaded_current();
    void ensure_current(AssetHandle handle);
    Reference<Asset> try_load_data(const AssetMetadata& metadata);

private:
    Thread thread;
    std::atomic<bool> running = true;

    // TODO: switch with concurrent queue
    std::queue<AssetMetadata> asset_loading_queue;
    std::mutex asset_loading_mutex;
    std::condition_variable asset_loading_cv;

    std::vector<AssetLoadResponse> loaded_assets_responses;
    std::mutex loaded_assets_responses_mutex;

    std::unordered_map<AssetHandle, Reference<Asset>> loaded_assets;
    std::mutex loaded_assets_mutex;

    WeakReference<AssetRegistry> asset_registry;

    float asset_update_perf = 0.f;
};

} // portal
