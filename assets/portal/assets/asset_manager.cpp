//
// Copyright © 2025 Jonatan Nevo.
// Distributed under the MIT license (see LICENSE file).
//

#include "asset_manager.h"

#include <unordered_set>

#include "asset.h"
#include "asset_source.h"
#include "../../../gui/portal/gui/texture.h"


namespace portal
{

static std::shared_ptr<AssetSource> create_asset_source(std::string_view asset_identifier)
{
#ifdef PORTAL_DIST
    if (asset_identifier.starts_with("https://"))
#else
    if (asset_identifier.starts_with("http://") || asset_identifier.starts_with("https://"))
#endif
    {
        // TODO: Validate if valid URL
        return std::make_shared<NetworkAssetSource>(asset_identifier);
    }

    // TODO: Check if relative and add base asset folder
    return std::make_shared<FileAssetSource>(asset_identifier);
}

AssetManager::AssetManager(const float asset_update_time)
    : asset_update_wait(asset_update_time)
{
    asset_loader_thread = std::jthread(
        [this](const std::stop_token& stoken)
        {
            load_assets(stoken);
        });
}

AssetManager::~AssetManager()
{
    stop();
}

void AssetManager::stop()
{
    asset_loader_thread.request_stop();
    if (asset_loader_thread.joinable())
        asset_loader_thread.join();
    assets.clear();
}

std::shared_ptr<Asset> AssetManager::get_asset(const std::string_view asset_identifier)
{
    const auto asset_source = create_asset_source(asset_identifier);
    return get_asset(asset_source);
}

std::shared_ptr<Asset> AssetManager::get_asset(std::shared_ptr<AssetSource> source)
{
    const uint32_t id = source->get_id();
    if (assets.contains(id))
        return assets[id];

    // TODO: Support immediate asset loading
    // TODO: Create correct asset based on source information
    assets[id] = std::make_shared<Texture>(TextureSpecification{.width = 256, .height = 256});
    assets[id]->source = source;
    asset_loading_queue.enqueue(std::move(source));

    return assets[id];
}

void AssetManager::update_assets(const float dt)
{
    AssetResponse response;
    while (assets_update_queue.try_dequeue(response))
    {
        const auto& asset = assets[response.id];
        asset->state = response.state;
        if (response.data)
            asset->set_data(response.data);
    }

    if (time_since_last_update < asset_update_wait)
    {
        time_since_last_update += dt;
        return;
    }

    // TODO: split to a different function if takes too much time?
    time_since_last_update = 0.f;
    auto asset_it = assets.begin();
    while (asset_it != assets.end())
    {
        if (asset_it->second.use_count() <= 1)
        {
            asset_it = assets.erase(asset_it);
        }
        else
            ++asset_it;
    }
}

void AssetManager::load_assets(const std::stop_token& stoken)
{
    std::shared_ptr<AssetSource> source = nullptr;
    while (!stoken.stop_requested())
    {
        if (asset_loading_queue.try_dequeue(source))
        {
            // TODO: Handle errors that are not missing assets
            const auto buffer = source->load_asset();
            AssetResponse response{
                .id = source->get_id(),
                .state = buffer ? AssetState::Loaded : AssetState::Missing,
                .data = buffer,
            };
            assets_update_queue.enqueue(response);
        }
        else // TODO: Should I sleep here?
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}


} // portal
