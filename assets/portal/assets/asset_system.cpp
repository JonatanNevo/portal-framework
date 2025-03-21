//
// Created by Jonatan Nevo on 21/03/2025.
//

#include "asset_system.h"

#include <ranges>

#include "portal/core/file_system.h"
#include "portal/core/timer.h"

namespace portal
{
using namespace std::chrono_literals;

AssetSystem::AssetSystem(): thread("Asset Thread")
{
    thread.dispatch([this]() { asset_thread_func(); });
}

AssetSystem::~AssetSystem()
{
    stop_and_wait();
}

void AssetSystem::set_registry(const Reference<AssetRegistry>& registry)
{
    asset_registry = WeakReference(registry);
}

void AssetSystem::queue_asset_load(const AssetMetadata& metadata)
{
    {
        std::scoped_lock lock (asset_loading_mutex);
        asset_loading_queue.push(metadata);
    }
    asset_loading_cv.notify_one();
}

Reference<Asset> AssetSystem::get_asset(const AssetMetadata& request)
{
    // Check if asset is already loaded in main asset manager
    {
        std::scoped_lock lock(loaded_assets_mutex);
        if (loaded_assets.contains(request.handle))
        {
            return loaded_assets.at(request.handle);
        }
    }

    // Check if asset has already been loaded but is pending sync back to asset manager.
    {
        std::scoped_lock lock(loaded_assets_responses_mutex);
        for (const auto& [metadata, asset] : loaded_assets_responses)
        {
            if (metadata.handle == request.handle)
            {
                return asset;
            }
        }
    }

    return try_load_data(request);
}

bool AssetSystem::retrieve_ready_assets(std::vector<AssetLoadResponse>& out_assets)
{
    PORTAL_CORE_ASSERT(out_assets.empty(), "out_assets should be empty");
    std::scoped_lock lock(loaded_assets_responses_mutex);
    std::swap(out_assets, loaded_assets_responses);

    // Now that we've synced assets, any events that were dispatched from try_load_data() are safe to be processed
    // This needs to be inside the loaded_assets_responses_mutex lock - we do not want any more evens to go into the queue before we've
    // marked all these as synced
    //Application::Get().SyncEvents();

    return !out_assets.empty();
}

void AssetSystem::update_loaded_asset_list(const std::unordered_map<AssetHandle, Reference<Asset>>& assets)
{
    std::scoped_lock lock(loaded_assets_mutex);
    loaded_assets = assets;
}

void AssetSystem::mark_asset_as_loaded(const AssetHandle& handle, const Reference<Asset>& asset)
{
    std::scoped_lock lock(loaded_assets_mutex);
    loaded_assets[handle] = asset;
}

void AssetSystem::stop()
{
    running = false;
    asset_loading_cv.notify_one();
}

void AssetSystem::stop_and_wait()
{
    stop();
    thread.join();
}

void AssetSystem::asset_monitor_update()
{
    Timer timer;
    timer.start();
    ensure_all_loaded_current();
    asset_update_perf = timer.stop();
}

void AssetSystem::asset_thread_func()
{
    // TODO: Add profiler
    //PORTAL_PROFILE_THREAD("Asset Thread");

    while (running)
    {
        //PORTAL_PROFILE_SCOPE("Asset Thread Queue");

        asset_monitor_update();

        bool queue_empty_or_stop = false;
        while (!queue_empty_or_stop)
        {
            AssetMetadata metadata;
            {
                std::scoped_lock lock(asset_loading_mutex);
                if (asset_loading_queue.empty() || !running)
                {
                    queue_empty_or_stop = true;
                }
                else
                {
                    metadata = asset_loading_queue.front();
                    asset_loading_queue.pop();
                }
            }

            // If queue_empty_or_stop then metadata will be invalid (handle == 0)
            // We check metadata here (instead of just breaking straight away on queue_empty_or_stop)
            // to deal with the edge case that other thread might queue requests for invalid assets.
            // This way, we just pop those requests and ignore them.
            if (metadata.is_valid())
                try_load_data(metadata);
        }

        std::unique_lock lock(asset_loading_mutex);
        // need to check conditions again, since other thread could have changed them between releasing the lock (in the while loop above)
        // and re-acquiring the lock here
        if (asset_loading_queue.empty() && running)
        {
            // need to wake periodically (here 100ms) so that asset_monitor_update() is called regularly to check for updated file timestamps
            // (which kinda makes condition variable redundant. may as well just sleep(100ms))
            asset_loading_cv.wait_for(lock, 100ms, [this]{return !running || !asset_loading_queue.empty();});
        }
    }
}

std::filesystem::path AssetSystem::get_filesystem_path(const AssetMetadata& metadata)
{
    // TODO: Get this path from config
    return std::filesystem::path("assets") / metadata.path;
}

void AssetSystem::ensure_all_loaded_current()
{
    //PORTAL_PROFILE_FUNC();

    // This is a long time to hold a lock.
    // However, copying the list of assets to iterate (so that we could then release the lock before iterating) is also expensive, so hard to tell which is better.
    std::scoped_lock lock(loaded_assets_mutex);
    for (const auto& handle : loaded_assets | std::views::keys)
    {
        ensure_current(handle);
    }
}

void AssetSystem::ensure_current(AssetHandle handle)
{
    AssetMetadata metadata{};
    if (asset_registry->contains(handle))
        metadata = asset_registry->get(handle);

    // other thread could have deleted the asset since our asset list was last synced
    if (!metadata.is_valid()) return;

    const auto absolute_path = get_filesystem_path(metadata);
    if (!FileSystem::exists(absolute_path))
        return;

    const uint64_t last_write_time = FileSystem::get_last_write_time(absolute_path);
    const uint64_t recorded_last_write_time = metadata.file_last_write_time;

    if (last_write_time == recorded_last_write_time)
        return;

    if (last_write_time == 0 || recorded_last_write_time == 0)
        return;

    return queue_asset_load(metadata);}

Reference<Asset> AssetSystem::try_load_data(const AssetMetadata& metadata)
{
    Reference<Asset> asset;
    if (!metadata.is_valid())
    {
        LOG_CORE_ERROR("Assets", "trying to load invalid asset: {}", metadata.path.string());
        return asset;
    }

    LOG_CORE_INFO_TAG("Assets", "{} Asset - {}", metadata.is_data_loaded? "Reloading" : "Loading", metadata.path.string());

    // // ASSUMPTION: AssetImporter serializers are immutable and re-entrant
    // if (AssetImporter::TryLoadData(metadata, asset))
    // {
    //     metadata.IsDataLoaded = true;
    //     auto absolutePath = GetFileSystemPath(metadata);
    //
    //     // Note (0x): There's a small hole here.  Other thread could start writing to asset's file in the exact instant that TryLoadData() has finished with it.
    //     //            GetLastWriteTime() then blocks until the write has finished, but now we have a new write time - not the one that was relevent for TryLoadData()
    //     //            To resolve this, you bascially need to lock the metadata until both the TryLoadData() _and_ the GetLastWriteTime() have completed.
    //     //            Or you need to update the last write time while you still have the file locked during TryLoadData()
    //     metadata.FileLastWriteTime = FileSystem::GetLastWriteTime(absolutePath);
    //     {
    //         std::scoped_lock<std::mutex> lock(m_LoadedAssetsMutex);
    //         m_LoadedAssets.emplace_back(metadata, asset);
    //
    //         // be careful:
    //         // 1) DispatchEvent() is only thread-safe when DispatchImmediately is false.
    //         // 2) Events must be handled carefully so that we are sure that the assets have been synched back to main thread _before_ this event is processed.
    //         //    That's why we are dispatching event while we hold a lock on m_LoadedAssetsMutex (see RetrieveReadyAssets())
    //         Application::Get().DispatchEvent<AssetReloadedEvent, /*DispatchImmediately=*/false>(metadata.Handle);
    //     }
    //
    //     HZ_CORE_INFO_TAG("AssetSystem", "Finished loading asset {}", metadata.FilePath.string());
    // }
    // else
    // {
    //     HZ_CORE_ERROR_TAG("AssetSystem", "Failed to load asset {} ({})", metadata.Handle, metadata.FilePath);
    // }

    return asset;
}
} // portal
