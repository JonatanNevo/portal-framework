//
// Created by thejo on 4/11/2025.
//

#pragma once

#include <memory>
#include <concurrentqueue/concurrentqueue.h>

#include "asset_types.h"
#include "portal/core/buffer.h"


namespace portal
{
class AssetSource;
class Asset;

class AssetManager
{
public:
    // TODO: remove this long timeout
    AssetManager(float asset_update_time = 100000.f);
    ~AssetManager();

    void stop();

    std::shared_ptr<Asset> get_asset(std::string_view asset_identifier);
    std::shared_ptr<Asset> get_asset(std::shared_ptr<AssetSource> source);

    // Should be called every frame, updates the pending assets from the `assets_update_queue`
    // and cleans stale assets, where ref count == 1 (only in the map)
    void update_assets(float dt);

private:
    struct AssetResponse
    {
        uint32_t id{};
        AssetState state{};
        Buffer data{};
    };

    void load_assets(const std::stop_token& stoken);

    float asset_update_wait;
    float time_since_last_update = 0.f;

    std::jthread asset_loader_thread;
    moodycamel::ConcurrentQueue<std::shared_ptr<AssetSource>> asset_loading_queue;
    moodycamel::ConcurrentQueue<AssetResponse> assets_update_queue;

    std::unordered_map<uint32_t, std::shared_ptr<Asset>> assets;
};

} // portal
